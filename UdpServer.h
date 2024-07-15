//canyon 2019 09 06

#pragma once

#include <stdio.h>
#include <map>
#include <string>
#include <list>

#include "Poco/Runnable.h"
#include "Poco/SharedPtr.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Timestamp.h"
#include "Poco/Thread.h"

#include "Poco/Net/Net.h"
#include "Poco/Net/DatagramSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/SocketAddressImpl.h"
#include "Poco/Net/Context.h"

#include "Data.h"

using namespace Poco::Net;

namespace CMM_SJY
{
class CUdpServer : public Poco::Runnable
{
	public:
		CUdpServer();
		~CUdpServer();

		int Start(int port);
		int Stop();
		bool ListenPortChange(int nPort);
		virtual void run();
	public:
		static bool m_bConnection;
	private:
		Poco::Net::DatagramSocket   m_ServerSocket;
		Poco::Thread m_thread;
		Poco::UInt16 m_listenPort;
		bool m_bStop;
	
};

}