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
		* ����xml���ݴ��
		*/
		static void EscapeData(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end, std::vector<uint8_t>& packet);
		/*
		* ����xml���ݴ��
		*/
		static void AntonymData(const std::vector<uint8_t>& data, int recvLen, std::vector<uint8_t>& packet);
		/*
		* �������ֵ ����headerLen�ֽڿ�ͷ ����tailLen�ֽڽ�β
		*/
		static uint8_t CalculateXORChecksum(const std::vector<uint8_t>& data, size_t headerLen, size_t tailLen);
		/*
		* ����xml���ݴ��
		*/
		static std::vector<uint8_t>  PackageSendData(std::vector<uint8_t>&  xmlData);
		/*
		* �����������
		*/
		static std::vector<uint8_t>  PackageSendHeart();
		/*
		* ���ܽ������
		*/
		static bool UnPackageRecvData(std::vector<uint8_t>& recvData, int recvBytes, std::vector<uint8_t>& outData);

	private:
	};
		
}
