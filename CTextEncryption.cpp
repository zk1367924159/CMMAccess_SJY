//canyon 2019 09 06

#include "CTextEncryption.h"
#include "CLog.h"
#include "openssl/sha.h"
//#include "openssl/openssl-1.1.1d/crypto/include/internal/sm3.h"
#include <string>
#include <sstream>
#include <iomanip>
#include "Poco/HMACEngine.h"
#include "openssl/aes.h"
#include "openssl/rand.h"
#include "openssl/rsa.h"
#include "openssl/pem.h"
#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/ec.h"
#include "openssl/hmac.h"  
#include "openssl/md5.h"

using namespace Poco::Net;

RSA* loadPublicKeyFromPEM(const CData& publicKeyPEM)
{
	BIO* bio = BIO_new_mem_buf(publicKeyPEM.c_str(), -1);
	if (!bio)
		throw std::runtime_error("Failed to create BIO");

	RSA* rsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
	if (!rsa)
	{
		BIO_free(bio);
		throw std::runtime_error("Failed to parse RSA public key");
	}

	BIO_free(bio);
	return rsa;
}

RSA* loadPrivateKeyFromPEM(const CData& privateKeyPEM)
{
	BIO* bio = BIO_new_mem_buf(privateKeyPEM.c_str(), -1);
	if (!bio)
		throw std::runtime_error("Failed to create BIO");

	RSA* rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
	if (!rsa)
	{
		BIO_free(bio);
		throw std::runtime_error("Failed to parse RSA public key");
	}

	BIO_free(bio);
	return rsa;
}

namespace CMM_SJY
{

	CTextEncryption::CTextEncryption()
	{
		
	}


	CTextEncryption::~CTextEncryption()
	{

	}

	
	bool CTextEncryption::hashMessage(const CData& inMessage, CData& outMessage,int nType)
	{
		// ����һ��stringstream���ڹ���ʮ�������ַ���
		std::stringstream ss;
		if (nType == 0)  //MD5
		{
			Poco::MD5Engine md5;
			md5.update(inMessage.c_str(), inMessage.length());
			outMessage = Poco::DigestEngine::digestToHex(md5.digest()).c_str();
		}
		else if (nType == 1) // SHA - 256(appkey)
		{
			unsigned char hash[SHA256_DIGEST_LENGTH];
			SHA256_CTX sha256;
			SHA256_Init(&sha256);
			SHA256_Update(&sha256, inMessage.c_str(), inMessage.length());
			SHA256_Final(hash, &sha256);
			for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
				ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i] & 0xff);
			}
			std::string token = ss.str();
			/*std::string outToken;
			for (char& c : token)
			{
				if (c >= 'a' && c <= 'f') {
					c = c - 'a' + 'A';
				}
				outToken += c;
			}*/
			outMessage = token;
		}
		else if (nType == 2)  //SM3
		{
			// ��ʼ��ԭʼ���ݻ�������SM3������
			//unsigned char hash[SM3_DIGEST_LENGTH];
			//SM3_CTX sm3;
			//// ��ʼ��������SM3������
			//sm3_init(&sm3);
			//sm3_update(&sm3, passwd.c_str(), passwd.length());
			//// ����SM3ɢ��ֵ
			//sm3_final(hash, &sm3);
			//// ��������ɢ��ֵת��Ϊ��д��ʮ�������ַ���
			//for (int i = 0; i < SM3_DIGEST_LENGTH; ++i) {
			//	ss << std::hex << std::uppercase << static_cast<int>(hash[i]);
			//}
			return false;
		}
		else
		{
			return false;
		}
		return true;
	}

	std::string CTextEncryption::toBase16(const unsigned char* data, size_t dataSize)
	{
		std::stringstream ss;
		for (size_t i = 0; i < dataSize; ++i) {
			ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
		}
		return ss.str();
	}

	// ���ַ���ת��ΪMD5ժҪ
	std::vector<unsigned char> CTextEncryption::MD5Hash(const CData& inputText)
	{
		unsigned char md5Buffer[EVP_MAX_MD_SIZE];
		MD5_CTX md5Context;

		MD5_Init(&md5Context);
		MD5_Update(&md5Context, inputText.c_str(), inputText.size());
		MD5_Final(md5Buffer, &md5Context);

		// ת��Ϊʮ�������ַ�����ʾ��ʽ
		return std::vector<unsigned char>(md5Buffer, md5Buffer + MD5_DIGEST_LENGTH);
	}

	// ʹ��MD5ժҪ��appkey����HMAC-SHA256
	std::string CTextEncryption::HMACSHA256(const std::string key, const std::string& md5Digest)
	{
		unsigned int hmacSize = EVP_MAX_MD_SIZE;
		unsigned char hmacResult[EVP_MAX_MD_SIZE];

		HMAC(EVP_sha256(), key.c_str(), key.length(),
			reinterpret_cast<const unsigned char*>(md5Digest.c_str()), md5Digest.length(),
			hmacResult, &hmacSize);

		return toBase16(hmacResult, hmacSize);
	}

	CData CTextEncryption::getToken(const CData& appKey, const CData& body)
	{
		std::vector<unsigned char> md5Digest = MD5Hash(body);
		std::string strMD5 = toBase16(md5Digest.data(), MD5_DIGEST_LENGTH);
		//LogInfo("md5Digest : " << strMD5);

		/*Poco::MD5Engine md5;
		md5.update(body.c_str(), body.length());
		CData outMessage = Poco::DigestEngine::digestToHex(md5.digest()).c_str();
		LogInfo("Poco MD5Engine : " << outMessage.c_str());*/

		std::string baseToken = HMACSHA256(appKey.c_str(), strMD5);
		//LogInfo("baseToken : " << baseToken.c_str());
		return baseToken;
	}


}

