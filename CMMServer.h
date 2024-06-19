#ifndef _CMMSERVER_H
#define _CMMSERVER_H
#include "Poco/Thread.h"
#include "Data.h"
#include "stdsoap2.h"
#include "soapH.h"
namespace CMM_SJY
{
	class CServer:public Poco::Runnable
	{
	private:
		CData m_endpoint;
		int m_serverPort;
		soap *m_soap;
		Poco::Thread m_thread;
		bool m_bStop;
		bool m_changePort;
	public:
		CServer();
		~CServer();
		void SetListenPort(int nPort);
		int BindSocket();
		int Start(int port, CData endpoint);		
		int Stop();
		virtual void run();
	};
}
#endif
