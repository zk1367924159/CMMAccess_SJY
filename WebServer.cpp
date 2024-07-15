//canyon 2019 09 06

#include "CMMDeviceConfig.h"
#include "WebServer.h"
#include "Poco/Timespan.h"
#include "Poco/Exception.h"
#include "Poco/ErrorHandler.h"
#include "CMMProtocolEncode.h"
#include "CMMAccess.h"

#include "Poco/Path.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/StreamCopier.h"

#include "fstream"

using namespace Poco::Net;



namespace CMM_SJY
{

	CWebRequestHandlerFactory::CWebRequestHandlerFactory()
	{

	}


	CWebRequestHandlerFactory::~CWebRequestHandlerFactory()
	{
	}

	CWebRequestHandler* CWebRequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request)
	{

		CWebRequestHandler* pRequestHandler = new CWebRequestHandler(this);
		{
			Poco::FastMutex::ScopedLock lock(m_connectionMapMutex);
			SocketAddress clientAddr = request.clientAddress();
			CData key = CData(clientAddr.host().toString()) + ":" + CData(clientAddr.port());
			m_HttpRequestHandlerMap[key] = pRequestHandler;
		}

		return pRequestHandler;
	}

	HTTPRequestHandler* CWebRequestHandlerFactory::GetHttpServerConnection(CData key)
	{
		Poco::FastMutex::ScopedLock lock(m_connectionMapMutex);
		auto it = m_HttpRequestHandlerMap.find(key);
		if (it != m_HttpRequestHandlerMap.end())
		{
			return it->second;
		}
		return nullptr;
	}


	void CWebRequestHandlerFactory::DelHttpServerConnection(CData key)
	{
		Poco::FastMutex::ScopedLock lock(m_connectionMapMutex);
		auto it = m_HttpRequestHandlerMap.find(key);
		if (it != m_HttpRequestHandlerMap.end())
		{
			m_HttpRequestHandlerMap.erase(it);
		}

	}


	CWebRequestHandler::CWebRequestHandler(void* owner)
	{
		m_owner = owner;
	}

	CWebRequestHandler::~CWebRequestHandler()
	{


	}

	void CWebRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	{
		response.set("Access-Control-Allow-Origin","*");
		response.set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
		response.set("Access-Control-Allow-Headers", "Content-Type");

		CData requestUri = request.getURI();
		CData method = request.getMethod().c_str();
		Poco::URI uri(requestUri.c_str());
		CData path = uri.getPath();
		std::string param = uri.getQuery();
		LogInfo("method : " << method << "  param :" << param);
		if (method == "GET")
		{
			if (path == "/GetDevice")
			{
				size_t nSize = param.find("=");
				if (nSize == std::string::npos)
				{
					response.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
					response.setContentType("application/json");
					std::ostream& out = response.send();
					out << CMMDeviceConfig::instance()->DecodeResponseJson(-1);
					return;
				}
				CData devID = param.substr(nSize + 1);
				LogInfo("devID : " << devID);
				std::string content = CMMDeviceConfig::instance()->GetDevJson(devID);
				LogInfo("content: " << content);
				response.setStatus(HTTPResponse::HTTP_OK);
				response.setContentType("application/json");
				std::ostream& out = response.send();
				out << content.c_str();
				return;
			}
			else if (path == "/GetAllDevice")
			{
			
				std::string content = CMMDeviceConfig::instance()->GetDevJson();
				LogInfo("content: " << content);
				response.setStatus(HTTPResponse::HTTP_OK);
				response.setContentType("application/json");
				std::ostream& out = response.send();
				out << content.c_str();
				return;
			}
			else
			{
				response.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
				response.setContentType("application/json");
				std::ostream& out = response.send();
				out << CMMDeviceConfig::instance()->DecodeResponseJson(-2);
				return;
			}
			//std::string filePath = "/appdata/extModule/webServer/index.html"; // 手动构建文件路径  
			//// 检查文件是否存在  
			//Poco::File file(filePath);
			//if (!file.exists() || !file.isFile() || !file.canRead())
			//{
			//	// 设置响应状态码和头部  
			//	response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
			//	response.setContentType("text/plain");
			//	response.send() << "html is not exist.";
			//	return;
			//}
			//// 发送文件内容作为响应体  
			//response.setStatus(HTTPResponse::HTTP_OK);
			//response.setContentType("text/html");
			//std::ifstream inFile(filePath, std::ios::binary);
			//Poco::StreamCopier::copyStream(inFile, response.send());
			//// 确保文件流被关闭  
			//inFile.close();
			return;
		}
		else if (method == "POST")
		{
			
			LogInfo("path: " << path);
			if (path == "/SetDevice")
			{
				std::istream& rs = request.stream();
				std::string content((std::istreambuf_iterator<char>(rs)),
					std::istreambuf_iterator<char>());
				LogInfo("content: " << content);
				if (-1 == CMMDeviceConfig::instance()->SetDevConf(content))
				{
					response.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
					response.setContentType("application/json");
					std::ostream& out = response.send();
					out << CMMDeviceConfig::instance()->EncodeResponseJson(-1);
					return;
				}
				CMMConfig::instance()->UpdateCfgFile();  //同步更新CMMConfig dev conf内容
				response.setStatus(HTTPResponse::HTTP_OK);
				response.setContentType("application/json");
				std::ostream& out = response.send();
				out << CMMDeviceConfig::instance()->EncodeResponseJson(0);
			}
			else
			{
				response.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
				response.setContentType("application/json");
				std::ostream& out = response.send();
				out << CMMDeviceConfig::instance()->EncodeResponseJson(-2);
				return;
			}
		}
		else if (method == "OPTIONS") 
		{
			response.setStatus(HTTPResponse::HTTP_OK);
			response.send();
			return;
		}
		else
		{
			response.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
			response.setContentType("application/json");
			std::ostream& out = response.send();
			out << CMMDeviceConfig::instance()->EncodeResponseJson(-2);
			return;
		}

		// 检查响应是否成功发送  
		if (response.sent())
		{
			LogNotice("Response send successfully.");
		}
		else
		{
			LogNotice("Failed to send response.");
			CWebServer::DisConnection();
		}
	}

	bool CWebServer::m_bConnection = true;

	CWebServer::CWebServer()
	{
		m_bStop = true;
		m_listenPort = -1;
	}


	CWebServer::~CWebServer()
	{
		Stop();
	}

	bool CWebServer::ListenPortChange(int nPort)
	{
		if (m_listenPort != nPort)
		{
			m_listenPort = nPort;
			m_bConnection = false;
			return true;
		}
		return false;
	}

	int CWebServer::Start(int port)
	{
		if (m_bStop == false)
		{
			LogError("cmm server has start");
			return -1;
		}
		m_bStop = false;
		m_listenPort = port;
		m_pParam = new HTTPServerParams();
		m_pFactory = new CWebRequestHandlerFactory();
		m_thread.start(*this);
		return 0;
	}

	void CWebServer::DeleteConnection(CData clientIp, int port)
	{
		CData key = clientIp;
		key += ":";
		key += CData(port);
		m_pFactory->DelHttpServerConnection(key);
	}

	int CWebServer::Stop()
	{
		if (m_bStop)
		{
			return -1;
		}
		m_bStop = true;
		// 停止HTTP服务器
		if (m_pHttpServer)
		{
			m_pHttpServer->stop();
		}
		// 等待线程结束（假设run方法会因m_bStop为true而退出循环）
		m_thread.join();
		// 清理资源
		if (m_pHttpServer)
		{
			delete m_pHttpServer;
			m_pHttpServer = nullptr;
		}
		m_ServerSocket.close();
		m_bConnection = true;
		m_bStop = true;
		return 0;
	}

	void CWebServer::run()
	{
		// 绑定端口并开始监听
		m_ServerSocket = ServerSocket(m_listenPort);
		m_ServerSocket.listen();
		m_pHttpServer = new HTTPServer(m_pFactory, m_ServerSocket, m_pParam);
		m_pHttpServer->start();
		while (m_bStop == false)
		{
			if (!m_bConnection)
			{
				try
				{
					if (m_pHttpServer)
					{
						m_pHttpServer->stop();
						delete m_pHttpServer;
						m_pHttpServer = nullptr;
						
					}
					// 绑定端口并开始监听
					m_ServerSocket.close();
					LogInfo("serverSocket listen port:" << m_listenPort);
					m_ServerSocket = ServerSocket(m_listenPort);
					m_ServerSocket.listen();
					m_pHttpServer = new HTTPServer(m_pFactory, m_ServerSocket, m_pParam);
					m_pHttpServer->start();
					m_bConnection = true;
					
				}
				catch (const Poco::Exception& e)
				{
					LogError("Server exception: " << e.displayText());
					m_bConnection = false;
					if (m_pHttpServer != nullptr) // 检查是否为nullptr以避免双重删除  
					{
						m_pHttpServer->stop();
						delete m_pHttpServer;
						m_pHttpServer = nullptr;
						m_ServerSocket.close();
					}
				}
			}
			else if (m_bStop)
			{
				break;
			}
			Poco::Thread::sleep(1000);
		}
		Stop();
	}

	void CWebServer::DisConnection()
	{
		m_bConnection = false;
	}
}






