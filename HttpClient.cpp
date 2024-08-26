//canyon 2019 09 06

#include "HttpClient.h"
#include "CLog.h"
#include "openssl/sha.h"
//#include "openssl/openssl-1.1.1d/crypto/include/internal/sm3.h"
#include <string>
#include <sstream>
#include <iomanip>
#include "Poco/HMACEngine.h"
#include "Poco/StreamCopier.h"

using namespace Poco::Net;


namespace CMM_SJY
{





	CHttpClient::CHttpClient()
	{

	}

	CHttpClient::~CHttpClient()
	{

	}

	void CHttpClient::Start()
	{
		//m_pContext = new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
	}

	int CHttpClient::SendXmlData(const char* url,CData xmlData, CData authHeader, HTTPResponse& response, CData& responseData)
	{
		Poco::URI uri(url);
		HTTPClientSession session(uri.getHost(), uri.getPort());
		CData m_strPath = uri.getPath();
		//LogInfo("host: " << uri.getHost() << " port: " << uri.getPort() << " path: " << m_strPath.c_str());
		LogInfo("send data to:"<< url << " header:" << authHeader.c_str());
		responseData = "";
		HTTPRequest request("POST", m_strPath.c_str(), "HTTP/1.1");
		request.setContentType("text/xml");
		request.setContentLength(xmlData.length());
		//request.set("Authorization", authHeader.c_str());
		//request.set("Accept", "text/xml, text/plain, */*");
		// 遍历并打印所有的HTTP头部
		/*for (auto it = request.begin(); it != request.end(); ++it)
		{
			LogInfo("Header: " << it->first << " = " << it->second);
		}*/
		try
		{
			std::ostream& os = session.sendRequest(request);
			if (!os) 
				throw std::runtime_error("Failed to send request");
			os << xmlData.c_str();
			os.flush();
		}
		catch (const Poco::TimeoutException& te) 
		{
			LogError("Timeout exception resquest: " << te.displayText());
			//m_pSession.reset();
			return -3;
		}
		catch (const Poco::Exception& e)
		{
			LogError("Error send resquest : " << e.displayText());
			return -1;
		}
		//LogInfo("send data sucess.");
		try
		{
			std::istream& rs = session.receiveResponse(response);
			std::string data;
			if (!rs) 
				throw std::runtime_error("Failed to recv response.");
			if (response.getStatus() == HTTPResponse::HTTPStatus::HTTP_OK)
			{
				Poco::StreamCopier::copyToString(rs, data);
				responseData = data;
			}
			else
			{
				LogError("Response status: " << response.getStatus() << " reason:" << response.getReason());
			}
			
			LogInfo("Response content: " << responseData.c_str());
			return response.getStatus();	
		}
		catch (const Poco::TimeoutException& te) {
			LogError("Timeout exception response: " << te.displayText());
			//m_pSession.reset();
			return -3;
		}
		catch (const Poco::Exception& e)
		{
			LogError("Error receive response: " << e.displayText());
			return -1;
		}
		return -2;
	}

	CData CHttpClient::GetTokenFromHeader(const HTTPResponse& response)
	{
		// 查找Authorization头并解析Bearer Token
		auto it = response.find("Authorization");
		std::string token, key, value;
		if (it != response.end()) {
			std::istringstream iss(it->second);
			while (std::getline(iss, key, '='))
			{
				std::getline(iss, value, ',');
				if (key == "token") {
					token = value.substr(1, value.length() - 2); // 去掉引号
					break;
				}
				iss.ignore(std::numeric_limits<std::streamsize>::max(), ','); // 跳过逗号和空格到下一个键值对

				iss.ignore(std::numeric_limits<std::streamsize>::max(), ' '); // 忽略逗号后可能存在的空格
			}
		}
		CData strToken(token);
		return strToken;
	}

}

