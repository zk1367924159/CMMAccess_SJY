//canyon 2019 09 06

#include "HttpServer.h"
#include "CMMSoapXmllEncode.h"
#include "Poco/Timespan.h"
#include "Poco/Exception.h"
#include "Poco/ErrorHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include "CMMProtocolEncode.h"
#include "CMMAccess.h"

using namespace Poco::Net;



namespace CMM_SJY
{


	const unsigned int CMCC_MAX_RESPONSE_BUFFER_SIZE = 512 * 1024;

	CHTTPRequestHandlerFactory::CHTTPRequestHandlerFactory()
	{

	}


	CHTTPRequestHandlerFactory::~CHTTPRequestHandlerFactory()
	{
	}

	CHTTPRequestHandler* CHTTPRequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request)
	{

		CHTTPRequestHandler* pRequestHandler = new CHTTPRequestHandler(this);
		{
			Poco::FastMutex::ScopedLock lock(m_connectionMapMutex);
			SocketAddress clientAddr = request.clientAddress();
			CData key = CData(clientAddr.host().toString()) + ":" + CData(clientAddr.port());
			m_HttpRequestHandlerMap[key] = pRequestHandler;
		}

		return pRequestHandler;
	}

	HTTPRequestHandler* CHTTPRequestHandlerFactory::GetHttpServerConnection(CData key)
	{
		Poco::FastMutex::ScopedLock lock(m_connectionMapMutex);
		auto it = m_HttpRequestHandlerMap.find(key);
		if (it != m_HttpRequestHandlerMap.end())
		{
			return it->second;
		}
		return nullptr;
	}


	void CHTTPRequestHandlerFactory::DelHttpServerConnection(CData key)
	{
		Poco::FastMutex::ScopedLock lock(m_connectionMapMutex);
		auto it = m_HttpRequestHandlerMap.find(key);
		if (it != m_HttpRequestHandlerMap.end())
		{
			m_HttpRequestHandlerMap.erase(it);
		}

	}


	CHTTPRequestHandler::CHTTPRequestHandler(void* owner)
	{
		m_owner = owner;
	}

	CHTTPRequestHandler::~CHTTPRequestHandler()
	{


	}

	//从http请求获取认证信息 再解析出token值
	std::string extractTokenFromCustomAuth(const std::string& customAuth) {
		std::istringstream iss(customAuth);
		std::string token;
		std::string field;
		//std::map<std::string, std::string> fields;
		while (std::getline(iss, field, ',')) 
		{
			std::size_t eqPos = field.find('=');
			if (eqPos != std::string::npos) {
				std::string key = field.substr(0, eqPos);
				std::string value = field.substr(eqPos + 1, field.size() - eqPos - 1); // 去掉等于号和两边的引号
				if (key == "token") 
				{
					value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
					token = value;
					break;
				}
				//fields[key] = value;
			}
		}

		return token;
	}

	void CHTTPRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
	{
		
		CData requestUri = request.getURI();
		CData method = request.getMethod().c_str();
		CData  auth_header, token, responseData;
		Poco::URI uri(requestUri.c_str());
		bool bstate = CMMAccess::instance()->GetLoginState();
		if (!bstate)
		{
			CData errorMsg = "Device not registered or registration expired";
			CMMAccess::instance()->UpdateAuthHeader(errorMsg, auth_header, token);
			response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
			response.setContentType("text/plain; charset=UTF-8");
			response.setContentLength(errorMsg.size());
			response.set("Authorization", auth_header.c_str());
			std::ostream& out = response.send();
			out << errorMsg.c_str();
			return;
		}
		if (method == "GET")
		{
			// 设置响应状态码和头部  
			response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
			response.setContentType("text/plain; charset=UTF-8");
			response.send();
			return;
		}
		else if (method == "POST")
		{
			if (uri.getPath() != "/services/FSUService")
			{
				CData errorMsg = "404 page not found";
				CMMAccess::instance()->UpdateAuthHeader(errorMsg, auth_header, token);
				response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
				response.setContentType("text/plain; charset=UTF-8");
				response.setContentLength(errorMsg.size());
				response.set("Authorization", auth_header.c_str());
				std::ostream& out = response.send();
				out << errorMsg.c_str();
				return;
			}
			if (request.getContentLength() == 0)
			{
				CData errorMsg = "Bad Request";
				CMMAccess::instance()->UpdateAuthHeader(errorMsg, auth_header, token);
				response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
				response.setContentType("text/plain; charset=UTF-8");
				response.setContentLength(errorMsg.size());
				response.set("Authorization", auth_header.c_str());
				std::ostream& out = response.send();
				out << errorMsg.c_str();
				return;
			}
			static char msgBuf[CMCC_MAX_RESPONSE_BUFFER_SIZE];
			//解析请求体
			std::string requestBody;
			try
			{
				// 直接读取整个响应体到字符串
				std::istream& rs = request.stream();
				requestBody.resize(static_cast<std::size_t>(request.getContentLength()));
				rs.read(&requestBody[0], requestBody.size());

				// 如果实际读取的字节数小于请求头中的Content-Length，说明可能读取不完整
				if (rs.gcount() != static_cast<std::streamsize>(requestBody.size()))
				{
					CData errorMsg = "Read xmlData Incomplete reading";
					CMMAccess::instance()->UpdateAuthHeader(errorMsg, auth_header, token);
					response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
					response.setContentType("text/plain; charset=UTF-8");
					response.setContentLength(errorMsg.size());
					response.set("Authorization", auth_header.c_str());
					std::ostream& out = response.send();
					out << errorMsg.c_str();
					return;
				}
				else
				{
					CData soapXmlData = requestBody.c_str();
					//LogInfo("recv SoapXmlData:" << requestBody.c_str());
					responseData = CMMSoapXmllEncode::setSoapDeserialization(soapXmlData);
					// 现在responseBody包含了整个XML内容，可以进行后续处理
					//LogInfo("recv xmlData:" << responseData.c_str());
					CMMAccess::instance()->UpdateAuthHeader(soapXmlData, auth_header, token);
					std::string strAuth = request.get("Authorization");
					CData requestToken = extractTokenFromCustomAuth(strAuth);
					//LogInfo("recv Authorization:" << strAuth.c_str());
					LogInfo("recv Auth token:" << requestToken.c_str() << " and Calculate the token:" << token.c_str());
					if (requestToken != token)
					{
						memset(msgBuf, 0, strlen(msgBuf));

						CMMAccess::instance()->DoMsgProcessError((char*)responseData.c_str(), msgBuf, (int)CMCC_MAX_RESPONSE_BUFFER_SIZE, 3);
						std::string  errorMsg = msgBuf;
						CData repSoapXml = CMMSoapXmllEncode::setSoapSerialization(errorMsg, 0);
						CMMAccess::instance()->UpdateAuthHeader(repSoapXml, auth_header, token);
						LogInfo("send data header:" << auth_header.c_str());
						response.setStatusAndReason(HTTPResponse::HTTP_OK);
						response.setContentType("text/xml; charset=UTF-8");
						response.setContentLength(repSoapXml.length());
						response.set("Authorization", auth_header.c_str());
						std::ostream& out = response.send();
						out.write(repSoapXml.c_str(), repSoapXml.length()); // 直接使用write方法发送缓冲区内容，避免字符串拷贝
						return;
					}
				}
			}
			catch (Poco::Exception& e)
			{
				// 处理异常情况
				CData errorMsg = "Bad Request";
				CMMAccess::instance()->UpdateAuthHeader(errorMsg, auth_header, token);
				response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
				response.setContentType("text/plain; charset=UTF-8");
				response.setContentLength(errorMsg.size());
				response.set("Authorization", auth_header.c_str());
				std::ostream& out = response.send();
				out << errorMsg.c_str();
				return;
			}
			try
			{
				memset(msgBuf, 0, strlen(msgBuf));
				int ret = CMMAccess::instance()->DoMsgProcess((char*)responseData.c_str(), msgBuf, (int)CMCC_MAX_RESPONSE_BUFFER_SIZE);
				if (ret < 0)
				{
					CData errorMsg = "request data is not able to be parsed";
					CMMAccess::instance()->UpdateAuthHeader(errorMsg, auth_header, token);
					response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
					response.setContentType("text/plain; charset=UTF-8");
					response.setContentLength(errorMsg.size());
					response.set("Authorization", auth_header.c_str());
					std::ostream& out = response.send();
					out << errorMsg.c_str();
					return;
				}
				std::string repXml = msgBuf;
				CData repSoapXml = CMMSoapXmllEncode::setSoapSerialization(repXml, 0);
				CMMAccess::instance()->UpdateAuthHeader(repSoapXml, auth_header, token);
				LogInfo("send data header:" << auth_header.c_str());
				response.setStatusAndReason(HTTPResponse::HTTP_OK);
				response.setContentType("text/xml; charset=UTF-8");
				response.setContentLength(repSoapXml.length());
				response.set("Authorization", auth_header.c_str());
				std::ostream& out = response.send();
				out.write(repSoapXml.c_str(), repSoapXml.length());
			}
			catch (Poco::Exception& e)
			{
				LogError("handleRequest caught an exception: name:" << e.name() << ", what:" << e.what() << ", text:" << e.displayText());
				// 异常情况下，发送一个通用的错误响应
				CData errorMsg = "internal server error";
				CMMAccess::instance()->UpdateAuthHeader(errorMsg, auth_header, token);
				response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
				response.setContentType("text/plain; charset=UTF-8");
				response.setContentLength(errorMsg.size());
				response.set("Authorization", auth_header.c_str());
				std::ostream& out = response.send();
				out << errorMsg.c_str();
			}
			catch (...)
			{
				LogError("handleRequest caught an unknown exception.");
				// 未知异常，发送一个通用的错误响应
				CData errorMsg = "Unknown error occurred";
				CMMAccess::instance()->UpdateAuthHeader(errorMsg, auth_header, token);
				response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
				response.setContentType("text/plain; charset=UTF-8");
				response.setContentLength(errorMsg.size());
				response.set("Authorization", auth_header.c_str());
				std::ostream& out = response.send();
				out << errorMsg.c_str();
			}
		}
		else
		{
			// 设置响应状态码和头部  
			response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
			response.setContentType("text/plain; charset=UTF-8");
			response.send();
		}
		

		// 检查响应是否成功发送  
		if (response.sent())
		{
			LogNotice("Response send successfully.");
		}
		else
		{
			LogNotice("Failed to send response.");
			CHttpServer::DisConnection();
		}
	}

	bool CHttpServer::m_bConnection = true;

	CHttpServer::CHttpServer()
	{
		m_bStop = true;
		m_listenPort = -1;
	}


	CHttpServer::~CHttpServer()
	{
		Stop();
	}

	bool CHttpServer::ListenPortChange(int nPort)
	{
		if (m_listenPort != nPort)
		{
			m_listenPort = nPort;
			m_bConnection = false;
			return true;
		}
		return false;
	}

	int CHttpServer::Start(int port, CData endpoint)
	{
		if (port == -1 || endpoint.empty())
		{
			LogError("cmm service endpoint an service port must be setted");
			return -1;
		}
		if (m_bStop == false)
		{
			LogError("cmm server has start");
			return -1;
		}
		m_bStop = false;
		// 创建并配置安全套接字
		m_listenPort = port;
		Poco::Net::Context::Ptr pContext = new Poco::Net::Context(Poco::Net::Context::SERVER_USE, CERTFILE, CERTFILE, CAFILE, Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
		Poco::Net::SSLManager::instance().initializeServer(nullptr, nullptr, pContext);
			// 配置HTTP服务器参数
		m_pParam = new HTTPServerParams();
		// 创建请求处理器工厂
		m_pFactory = new CHTTPRequestHandlerFactory();
		// 创建并启动HTTPS服务器
		m_thread.start(*this);
		return 0;
	}

	void CHttpServer::DeleteConnection(CData clientIp, int port)
	{
		CData key = clientIp;
		key += ":";
		key += CData(port);
		m_pFactory->DelHttpServerConnection(key);
	}

	int CHttpServer::Stop()
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

	void CHttpServer::run()
	{
		// 绑定端口并开始监听
		m_ServerSocket = SecureServerSocket(m_listenPort);
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
					m_ServerSocket = SecureServerSocket(m_listenPort);
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

	void CHttpServer::DisConnection()
	{
		m_bConnection = false;
	}
}






