//canyon 2019 09 06

#include "UdpServer.h"
#include "TransData.h"
#include "Poco/Timespan.h"
#include "Poco/Exception.h"
#include "Poco/ErrorHandler.h"
#include "CMMProtocolEncode.h"
#include "CMMAccess.h"

using namespace Poco::Net;

namespace CMM_SJY
{


	const unsigned int CMCC_MAX_RESPONSE_BUFFER_SIZE = 512 * 1024;

	CUdpServer::CUdpServer()
	{
		m_bStop = true;
		m_listenPort = -1;
	}


	CUdpServer::~CUdpServer()
	{
		Stop();
	}

	bool CUdpServer::ListenPortChange(int nPort)
	{
		if (m_listenPort != nPort)
		{
			m_listenPort = nPort;
			return true;
		}
		return false;
	}

	int CUdpServer::Start(int port)
	{
		if (port == -1)
		{
			LogError("cmm service endpoint an service port must be setted");
			return -1;
		}
		m_bStop = false;
		m_listenPort = port;
		m_thread.start(*this);
		return 0;
	}

	int CUdpServer::Stop()
	{
		if (m_bStop)
		{
			return -1;
		}
		// 等待线程结束（假设run方法会因m_bStop为true而退出循环）
		m_thread.join();
		
		m_ServerSocket.close();
		m_bStop = true;
		return 0;
	}

	void CUdpServer::run()
	{
		// 绑定端口并开始监听
		SocketAddress serverAddr(m_listenPort);
		m_ServerSocket.bind(serverAddr);
		LogInfo("udp server listen port : " << m_listenPort);
		while (m_bStop == false)
		{
			// 接收数据，尝试简单重传逻辑
			bool received = false;
			int recvBytes = 0;
			int nCount = 0;
			std::vector<uint8_t> recvBuffer(1024); // 初始大小为1024  
			SocketAddress senderAddr;
			try
			{
				while (true)
				{
					recvBytes = m_ServerSocket.receiveFrom(recvBuffer.data(), recvBuffer.size(), senderAddr);
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
						LogError("recv ip: " << senderAddr.host() << " data is so Larger. please check data retry.");
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
					std::string response = "Failed to receive data after retries.";
					m_ServerSocket.sendTo(response.c_str(), response.length(), senderAddr);
					LogError("Failed to receive data after retries.");
					continue;
				}
				std::vector<uint8_t> outBuffer;
				outBuffer.reserve(recvBytes);
				if (!CTransData::UnPackageRecvData(recvBuffer, recvBytes, outBuffer))
				{
					std::string response = "UnPackageRecvData failed.";
					m_ServerSocket.sendTo(response.c_str(), response.length(), senderAddr);
					LogError("UnPackageRecvData return false.");
					continue;
				}
				LogInfo("recvBytes: " << recvBytes << " outBuffer size:" << outBuffer.size());
				if (!CMMAccess::instance()->writeDataToUart(outBuffer))
				{
					std::string response = "writeDataToUart failed.";
					m_ServerSocket.sendTo(response.c_str(), response.length(), senderAddr);
					LogError("writeDataToUart false.");
					continue;
				}
				std::string response = "writeDataToUart sucess.";
				m_ServerSocket.sendTo(response.c_str(), response.length(), senderAddr);
			}
			catch (Poco::Exception& exc)
			{
				LogError("Exception msg: " << exc.displayText());
			}
		}
		Stop();
	}
}






