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

	//��http�����ȡ��֤��Ϣ �ٽ�����tokenֵ
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
				std::string value = field.substr(eqPos + 1, field.size() - eqPos - 1); // ȥ�����ںź����ߵ�����
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
			// ������Ӧ״̬���ͷ��  
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
			//����������
			std::string requestBody;
			try
			{
				// ֱ�Ӷ�ȡ������Ӧ�嵽�ַ���
				std::istream& rs = request.stream();
				requestBody.resize(static_cast<std::size_t>(request.getContentLength()));
				rs.read(&requestBody[0], requestBody.size());

				// ���ʵ�ʶ�ȡ���ֽ���С������ͷ�е�Content-Length��˵�����ܶ�ȡ������
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
					// ����responseBody����������XML���ݣ����Խ��к�������
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
						out.write(repSoapXml.c_str(), repSoapXml.length()); // ֱ��ʹ��write�������ͻ��������ݣ������ַ�������
						return;
					}
				}
			}
			catch (Poco::Exception& e)
			{
				// �����쳣���
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
				// �쳣����£�����һ��ͨ�õĴ�����Ӧ
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
				// δ֪�쳣������һ��ͨ�õĴ�����Ӧ
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
			// ������Ӧ״̬���ͷ��  
			response.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
			response.setContentType("text/plain; charset=UTF-8");
			response.send();
		}
		

		// �����Ӧ�Ƿ�ɹ�����  
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
		// ���������ð�ȫ�׽���
		m_listenPort = port;
		Poco::Net::Context::Ptr pContext = new Poco::Net::Context(Poco::Net::Context::SERVER_USE, CERTFILE, CERTFILE, CAFILE, Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
		Poco::Net::SSLManager::instance().initializeServer(nullptr, nullptr, pContext);
			// ����HTTP����������
		m_pParam = new HTTPServerParams();
		// ����������������
		m_pFactory = new CHTTPRequestHandlerFactory();
		// ����������HTTPS������
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
		// ֹͣHTTP������
		if (m_pHttpServer)
		{
			m_pHttpServer->stop();
		}
		// �ȴ��߳̽���������run��������m_bStopΪtrue���˳�ѭ����
		m_thread.join();
		// ������Դ
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
		// �󶨶˿ڲ���ʼ����
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
					// �󶨶˿ڲ���ʼ����
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
					if (m_pHttpServer != nullptr) // ����Ƿ�Ϊnullptr�Ա���˫��ɾ��  
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






