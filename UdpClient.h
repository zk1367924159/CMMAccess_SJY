//canyon 2019 09 06

#pragma once

#include <stdio.h>
#include "Poco/SharedPtr.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Timestamp.h"
#include "Data.h"


#include "Poco/Net/DatagramSocket.h"
#include "Poco/Net/SocketAddress.h" 
#include "Poco/Net/SocketAddressImpl.h"
#include "Poco/URI.h"
#include "Poco/DigestStream.h"

using namespace Poco::Net;


namespace CMM_SJY
{

	class SingletonSocket {
	public:
		static SingletonSocket& instance() {
			static SingletonSocket singleton; // 静态局部变量，线程安全初始化
			return singleton;
		}

		DatagramSocket& getSocket() {
			return socket_;
		}

	private:
		DatagramSocket socket_;

		SingletonSocket() {
			socket_.bind(SocketAddress(SocketAddress::IPv4, "0.0.0.0", 0));
		}

		~SingletonSocket() {
			socket_.close();
		}

		SingletonSocket(const SingletonSocket&) = delete;
		SingletonSocket& operator=(const SingletonSocket&) = delete;
	};

	class CUdpClient 
	{

	public:
		CUdpClient();
		~CUdpClient();
		void Start();
		/*
		* 发送串口数据 返回1成功 -2超时 其他失败
		*/
		int SendData(const char* url, std::vector<uint8_t>& uartData);
		/*
		* 发送心跳
		*/
		int SendHeart(const char* url);
	private:
		std::string m_pUser;
	};
		
}
