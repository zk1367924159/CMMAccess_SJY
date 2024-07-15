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

#define MAX_RECV_DATASIZE  2* 1024 * 1024

namespace CMM_SJY
{

	class CTransData
	{

	public:
		CTransData();
		~CTransData();
		/*
		* 发送xml数据打包
		*/
		static void EscapeData(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, std::vector<uint8_t>& packet);
		/*
		* 发送xml数据打包
		*/
		static void AntonymData(const std::vector<uint8_t>& data, int recvLen, std::vector<uint8_t>& packet);
		/*
		* 计算异或值 跳过headerLen字节开头 跳过tailLen字节结尾
		*/
		static uint8_t CalculateXORChecksum(const std::vector<uint8_t>& data, size_t headerLen, size_t tailLen);
		/*
		* 发送xml数据打包
		*/
		static std::vector<uint8_t>  PackageSendData(std::vector<uint8_t>&  xmlData);
		/*
		* 发送心跳打包
		*/
		static std::vector<uint8_t>  PackageSendHeart();
		/*
		* 接受解包数据
		*/
		static bool UnPackageRecvData(std::vector<uint8_t>& recvData, int recvBytes, std::vector<uint8_t>& outData);

	private:
	};
		
}
