//canyon 2019 09 06

#include "UdpClient.h"
#include "TransData.h"
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

	CUdpClient::CUdpClient()
	{

	}

	CUdpClient::~CUdpClient()
	{

	}

	void CUdpClient::Start()
	{
		//m_pUser = url;
	}

	int CUdpClient::SendData(const char* url, std::vector<uint8_t>& uartData)
	{
		Poco::URI uri(url);
		std::string serverAddress = uri.getHost();
		int serverPort = uri.getPort();
		LogInfo("SEND data to serverAddress "<< serverAddress << " and serverPort:"<< serverPort);
		try
		{
			SocketAddress serverAddr(SocketAddress::IPv4, serverAddress, serverPort);
			auto& socket = SingletonSocket::instance().getSocket();
			socket.setReceiveTimeout(Poco::Timespan(5, 0));
			std::vector<uint8_t> sendData = CTransData::PackageSendData(uartData);
			int sentBytes = socket.sendTo(sendData.data(), sendData.size(), serverAddr);
			if (sentBytes < 0)
			{
				LogError("send msg error: " << sentBytes);
				return -1;
			}
			// 接收数据，尝试简单重传逻辑
			bool received = false;
			int recvBytes = 0;
			int nCount = 0;
			std::vector<uint8_t> recvBuffer(1024); // 初始大小为1024  
			while (true) 
			{
				SocketAddress senderAddr;
				recvBytes = socket.receiveFrom(recvBuffer.data(), recvBuffer.size(), senderAddr);
				if (recvBytes <= 0)
				{	
					nCount++;
				}
				else if (nCount > 3)
				{
					received = false;
					LogError("recv ip: " << senderAddr.host() << " data recv error. please check data retry.");
					break;
				}
				else if (recvBytes == (int)recvBuffer.size()) 
				{
					// 缓冲区可能不足以容纳所有数据，增加缓冲区大小  
					recvBuffer.resize(recvBuffer.size() * 2); // 示例：加倍缓冲区大小  
				}
				else if (recvBuffer.size() >= MAX_RECV_DATASIZE)
				{
					received = false;
					LogError("recv ip: "<< senderAddr.host()<<" data is so Larger. please check data retry.");
					break;
				}
				else 
				{
					received = true;
					break; // 收到小于缓冲区大小的数据，或发生错误  
				}
			}
			if (!received)
			{
				LogError("Failed to receive data after retries.");
				return -2;
			}
			std::string strLen(recvBuffer.begin(), recvBuffer.end());
			LogInfo("recv server response:" << strLen.c_str());
			/*std::string strRecv;
			if (!CTransData::UnPackageRecvData(recvBuffer, recvBytes, strRecv))
			{
				LogError("Failed to :UnPackageRecvData.");
				return -3;
			}
			recvData = strRecv.c_str();*/
		}
		catch (Poco::Exception& exc)
		{
			LogError("Exception msg: " << exc.displayText());
			return -4;
		}
		return 0;
	}

	int CUdpClient::SendHeart(const char* url)
	{
		Poco::URI uri(url);
		std::string serverAddress = uri.getHost();
		int serverPort = uri.getPort();
		LogInfo("SEND Heart to serverAddress " << serverAddress << " and serverPort:" << serverPort);
		try
		{
			
			SocketAddress serverAddr(SocketAddress::IPv4, serverAddress, serverPort);
			auto& socket = SingletonSocket::instance().getSocket();
			socket.setReceiveTimeout(Poco::Timespan(5, 0));
			std::vector<uint8_t> sendData = CTransData::PackageSendHeart();
			int sentBytes = socket.sendTo(sendData.data(), sendData.size(), serverAddr);
			if (sentBytes < 0)
			{
				LogError("send msg error: " << sentBytes);
				return -1;
			}
			SocketAddress senderAddr;
			std::vector<uint8_t> recvBuffer(200,0); // 初始大小为1024  
			bool received = false;
			while (true) 
			{
				int recvBytes = socket.receiveFrom(recvBuffer.data(), recvBuffer.size(), senderAddr);
				if (recvBytes <= 0)
				{
					received = false;
					break;
				}
				else
				{
					received = true;
					break; // 收到小于缓冲区大小的数据，或发生错误  
				}	
			}
			if (!received)
			{
				LogError("Failed to receive data after retries.");
				return -2;
			}
			std::string strLen(recvBuffer.begin(), recvBuffer.end());
			LogInfo("recv heart:" << strLen.c_str());
		}
		catch (Poco::Exception& exc)
		{
			LogError("Exception msg: " << exc.displayText());
			return -4;
		}
		return 0;
	}
}

