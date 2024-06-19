//canyon 2019 09 06

#pragma once

#include <stdio.h>
#include <map>
#include <string>
#include <list>



#include "Poco/Net/TCPServer.h"

#include "Poco/Net/SocketAddress.h"
#include "Poco/Runnable.h"
#include "Poco/SharedPtr.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Timestamp.h"
#include "Poco/Thread.h"

#include "Poco/Net/Net.h"
#include "Poco/Net/SecureServerSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerResponse.h"  
#include "Poco/Net/HTTPServerRequest.h" 
#include "Poco/Net/Context.h"

#include "Data.h"

using namespace Poco::Net;

namespace CMM_SJY
{

class CHTTPRequestHandler : public HTTPRequestHandler
{
	public:
		CHTTPRequestHandler(void* owner);

		virtual ~CHTTPRequestHandler();
			
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response);
	private:
		bool _stopped;
		Poco::FastMutex _mutex;
		void* m_owner;
};

class CHTTPRequestHandlerFactory : public HTTPRequestHandlerFactory
{
	public:
		CHTTPRequestHandlerFactory();

		~CHTTPRequestHandlerFactory();

		CHTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request);

		void DelHttpServerConnection(CData key);
		HTTPRequestHandler* GetHttpServerConnection(CData key);
		
	private:
		Poco::FastMutex m_connectionMapMutex;
		std::map<CData, HTTPRequestHandler*> m_HttpRequestHandlerMap;
};

class CHttpServer : public Poco::Runnable
{
	public:
		CHttpServer();
		~CHttpServer();
		int Start(int port, CData endpoint);
		int Stop();
		void DeleteConnection(CData clientIp, int port);
		static void DisConnection();
		bool ListenPortChange(int nPort);
		virtual void run();
	public:
		static bool m_bConnection;
	private:
		Poco::Net::ServerSocket  m_ServerSocket;
		Poco::Net::HTTPServerParams::Ptr m_pParam;
		Poco::Net::HTTPServer* m_pHttpServer;
		Poco::Thread m_thread;
		Poco::SharedPtr<CHTTPRequestHandlerFactory> m_pFactory;
		Poco::UInt16 m_listenPort;
		bool m_bStop;
	
};

}