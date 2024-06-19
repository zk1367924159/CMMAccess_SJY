#include "CMMServer.h"
#include "CLog.h"
#include "SysCommon.h"
#include "CMMAccess.h"

#include "openssl/ssl.h"  // OpenSSL库的SSL/TLS功能

extern struct Namespace cmmNamespaces[];
// OpenSSL 的总是验证通过的回调函数示例

namespace CMM_SJY
{

	CServer::CServer()
	{
	
		m_bStop = true;
		m_soap = soap_new2(SOAP_XML_IGNORENS, SOAP_XML_IGNORENS);
		soap_init(m_soap);
		soap_set_mode(m_soap, SOAP_C_UTFSTRING);
		//m_soap->ssl_flags |= SOAP_SSL_NO_AUTHENTICATION; // No authentication
		m_soap->connect_timeout = 5;
		m_soap->accept_timeout = 20;
		m_soap->recv_timeout = 3;
		m_soap->send_timeout = 3;
		m_soap->namespaces = cmmNamespaces;
		
		m_serverPort = -1;
		m_changePort = false;
	}

	CServer::~CServer()
	{

	}

	void CServer::SetListenPort(int nPort)
	{
		if (m_serverPort != nPort)
		{
			m_serverPort = nPort;
			m_changePort = true;
		}		
	}

	int CServer::Start( int port, CData endpoint )
	{
		if (port ==-1 || endpoint.empty())
		{
			LogError("cmm service endpoint an service port must be setted");
			return -1;
		}
		if(m_bStop == false)
		{			
			LogError("cmm server has start");
			return -1;
		}
		soap_ssl_init();
		if (soap_ssl_server_context(m_soap,
			SOAP_TLSv1_2| SOAP_TLSv1_3,// 使用默认的SSL方法
			CERTFILE, // 服务器私钥文件路径
			nullptr, // 私钥密码，如果有的话
			CAFILE, // 自签名服务器证书文件路径
			nullptr, // cacert，这里为空表示不验证客户端证书
			nullptr, // capath，OpenSSL中的证书目录路径，这里不需要
			nullptr, // dhfile，Diffie-Hellman参数文件路径，可选
			nullptr) != SOAP_OK)
			
		{ 
			LogError("soap_print_fault error.");
			soap_print_fault(m_soap, stderr);
			soap_destroy(m_soap);
			soap_end(m_soap);
			return -1;
		}
		m_bStop = false;
		m_serverPort = port;
		m_endpoint = endpoint;
		m_thread.start(*this);
		return 0;
	}

	int CServer::BindSocket()
	{
		SOAP_SOCKET ssock = soap_bind(m_soap, NULL, m_serverPort, 10);
		if (!soap_valid_socket(ssock))
		{
			LogFatal("cmm soap service start failed on port:" << m_serverPort<< " error:"<<m_soap->errnum);
			CMMAccess::LogSoapError(m_soap);
			//ISFIT_OS::exit_on_fatal(ISFIT_OS::TCP_LISTEN_FAILED);
			return -1;
		}
		LogInfo("cmm soap server start success on " << m_endpoint << " and port " << m_serverPort);
		
		return 0;
	}

	int CServer::Stop()
	{
		if(m_bStop)
		{
			return -1;
		}
		m_bStop = true;
		soap_cleanup(m_soap);
		m_thread.join();
		soap_done(m_soap);
		return 0;
	}

	void CServer::run()
	{
		int ret=BindSocket();
		while (m_bStop == false)
		{
			if(ret == -1)
			{
				Poco::Thread::sleep(20000);
				soap_destroy(m_soap);
				soap_end(m_soap);
				ret = BindSocket();
				continue;
			}
			if (m_changePort == true)
			{
				soap_destroy(m_soap);
				soap_end(m_soap);
				ret = BindSocket();
				m_changePort = false;
				continue;
			}
			if(soap_valid_socket(soap_accept(m_soap)))
			{
				if (soap_valid_socket(soap_ssl_accept(m_soap)))
				{
					// 服务端接受一个新的连接
					if (soap_serve(m_soap) != SOAP_OK)
					{
						//CTowerAccess::LogSoapError(m_soap);
					}
				}
				soap_destroy(m_soap);
				soap_end(m_soap);
			}
			else
			{
				Poco::Thread::sleep(100);
			}
		}
	}
}


