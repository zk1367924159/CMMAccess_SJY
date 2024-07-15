//canyon 2019 09 06

#include "TransData.h"
#include "CMMConfig.h"
#include "CMMAccess.h"
#include "CLog.h"
//#include "openssl/openssl-1.1.1d/crypto/include/internal/sm3.h"
#include <string>
#include <sstream>
#include <string> 
namespace CMM_SJY
{





	CTransData::CTransData()
	{

	}

	CTransData::~CTransData()
	{

	}

	// ����һ������������ת��  
	void CTransData::EscapeData(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end ,std::vector<uint8_t>& packet)
	{
		while (start != end) 
		{
			switch (*start)
			{
			case 0xFF:
				// ����ת������ǣ�����0xFF���ȼ�ת���ַ�0x7D��Ȼ��0xFF��Ϊ0xFF+1=0x00  
				packet.push_back(0xFD);
				packet.push_back(0x00);
				break;
			case 0xFE:
				// ��������0xFE���ȼ�ת���ַ�0x7D��Ȼ��0xFE��Ϊ0xFE+1=0xFF��ע��������ܻ���0xFF��ת���ͻ������ȡ����Э�飩  
				// ���ߣ����Э���в�ͬ�Ĺ��򣬱���ֱ�ӱ�Ϊ��һ���̶�ֵ����������任  
				packet.push_back(0xFD);
				packet.push_back(0xFF); // ����ֱ�ӱ�Ϊ0xFF������ȡ����Э��  
				break;
			case 0xFD:
				// ���Ƶش���0xFD  
				packet.push_back(0xFD);
				packet.push_back(0x02); // ����ֻ��ʾ����ʵ��ֵȡ����Э��  
				break;
			default:
				// �����ֽ�ֱ�����  
				packet.push_back(*start);
			}
			++start;
		}
	}

	// ����һ������������ת��  
	void CTransData::AntonymData(const std::vector<uint8_t>& data, int recvLen ,std::vector<uint8_t>& packet)
	{
		int i = 0;
		while (i < recvLen)
		{
			if (data[i] == 0xFD)  // ��ת���ַ���ʼ  
			{
				if (i + 1 >= recvLen)
				{
					throw std::runtime_error("Incomplete escape sequence in data");
				}

				switch (data[i + 1]) 
				{
				case 0x00: // ��ԭ0xFF  
					packet.push_back(0xFF);
					i += 2; // ����ת������  
					break;
				case 0x01: // ��ԭ0xFE����������Э��涨�ģ�  
					packet.push_back(0xFE);
					i += 2; // ����ת������  
					break;
				case 0x02: // ��ԭ0xFD������ʹ��0x02��Ϊʾ��������ֵȡ����Э�飩  
					packet.push_back(0xFD);
					i += 2; // ����ת������  
					break;
				default:
					throw std::runtime_error("Invalid escape sequence in data");
				}
			}
			else 
			{
				// ����ת���ַ���ֱ����ӵ������  
				packet.push_back(data[i]);
				i++;
			}
		}
	}

	// �������У��ֵ  
	uint8_t CTransData::CalculateXORChecksum(const std::vector<uint8_t>& data, size_t headerLen, size_t tailLen) {
		uint8_t checksum = 0;
		size_t dataSize = data.size();
		if (dataSize >= headerLen + tailLen) 
		{
			// �Գ��˰�ͷ�Ͱ�β֮������ݽ���������  
			for (size_t i = headerLen; i < dataSize - tailLen; ++i) 
			{
				checksum ^= data[i];
			}
		}
		return checksum;
	}

	std::vector<uint8_t> CTransData::PackageSendData(std::vector<uint8_t>& uartData)
	{
		
		int nMaxPackageSize = 40 + uartData.size();
		std::vector<uint8_t>    packet;
		packet.reserve(nMaxPackageSize);
		packet.push_back(0xFF);
		for(int i = 0; i < 8; ++i)
			packet.push_back(0x00);          //sc��ַ
		
		CData fsuId = CMMConfig::instance()->GetFsuId();
		if (uartData.size() <= 0 || fsuId.size() <= 0)
		{
			LogError("sendData length is so short : "<< uartData.size() <<"  or fsuid is null.");
			throw std::runtime_error("An error occurred while getting data");
		}
			
		// ��fsuid�����ݸ��Ƶ�vector��  
		for (int i = 0; i < 20 ; ++i)
		{
			if(i < fsuId.size())
				packet.push_back(static_cast<uint8_t>(fsuId[i]));   //fsuid
			else
				packet.push_back(0x00);   //fsuid
		}

		packet.push_back(0x01);    //���豸����
		uint8_t p_uart = ((CMMAccess::instance()->m_uartService->getUartID() & 0xf) << 4) | (CMMAccess::instance()->m_uartService->m_slaveID & 0xf);
		LogInfo("p_uart: " << (int)p_uart);
		packet.push_back(p_uart);    //���ں�+��ַ��

		uint16_t p_len = 5 + uartData.size();
		
		packet.push_back(p_len & 0xff);  //Э�������ݰ�����
		packet.push_back((p_len >> 8) & 0xff);

		packet.push_back(0x00);   //Ӧ������
		uint16_t command_id = 0x0001;
		
		packet.push_back(command_id & 0xff);  //������
		packet.push_back((command_id >> 8) & 0xff);

		uint16_t t_len = static_cast<uint16_t>(uartData.size());
		packet.push_back(t_len & 0xff); //xmldata���ݳ���
		packet.push_back((t_len >> 8) & 0xff);

		for (uint16_t i = 0; i < t_len ; ++i)
		{
			packet.push_back(uartData[i]);   //xmldata
		}
		uint8_t p_verify = CalculateXORChecksum(packet, 1 ,0);
		packet.push_back(p_verify);   //У��
		packet.push_back(0xFE);   //��β

		std::vector<uint8_t>  packet_out;
		packet_out.push_back(0xFF);
		EscapeData(packet.begin() + 1, packet.end()-1, packet_out); ///ת��
		packet_out.push_back(0xFE);   //��β
		return packet_out;
	}

	std::vector<uint8_t> CTransData::PackageSendHeart()
	{
		int nMaxPackageSize = 38;
		std::vector<uint8_t>    packet;
		packet.reserve(nMaxPackageSize);
		packet.push_back(0xFF);
		for (int i = 0; i < 8; ++i)
			packet.push_back(0x00);          //sc��ַ

		CData fsuId = CMMConfig::instance()->GetFsuId();
		if (fsuId.size() <= 0)
		{
			LogError("fsuid is null.");
			throw std::runtime_error("An error occurred while getting data");
		}

		// ��fsuid�����ݸ��Ƶ�vector��  
		for (int i = 0; i < 20; ++i)
		{
			if (i < fsuId.size())
				packet.push_back(static_cast<uint8_t>(fsuId[i]));   //fsuid
			else
				packet.push_back(0x00);   //fsuid
		}

		packet.push_back(0x01);    //���豸����
		uint8_t p_uart = ((CMMAccess::instance()->m_uartService->getUartID() & 0xf) << 4) | (CMMAccess::instance()->m_uartService->m_slaveID & 0xf);
		LogInfo("p_uart: " << (int)p_uart);
		packet.push_back(p_uart);    //���ں�+��ַ��

		uint16_t p_len = 3;
		//packet.push_back((p_len >> 8) & 0xff);
		//packet.push_back(p_len & 0xff);  //Э�������ݰ�����
		packet.push_back(p_len & 0xff);  //Э�������ݰ�����
		packet.push_back((p_len >> 8) & 0xff);
		

		packet.push_back(0xED);   //RtnFlag
		uint16_t command_id = 0x0002;
		
		packet.push_back(command_id & 0xff);  //������
		packet.push_back((command_id >> 8) & 0xff);

		uint8_t p_verify = CalculateXORChecksum(packet, 1, 0);
		packet.push_back(p_verify);   //У��
		packet.push_back(0xFE);   //��β

		std::vector<uint8_t>  packet_out;
		packet_out.push_back(0xFF);
		EscapeData(packet.begin()+1 ,packet.end()-1, packet_out); ///ת��
		packet_out.push_back(0xFE);   //��β
		return packet_out;
	}

	bool CTransData::UnPackageRecvData(std::vector<uint8_t>& recvData,int recvLen, std::vector<uint8_t>& outData)
	{
		if (recvLen < 40)
		{
			LogError("recvData length is so short :" << recvLen);
			return false;
		}
		uint8_t p_header = recvData[0];
		if (p_header != 0xFF || recvData[recvLen - 1] != 0xFE)  //�ж��Ƿ��п�ʼ�ͽ�����ʶ
		{
			LogError("p_header is not 0xff :" << (int)p_header << " or p_end is not 0xFE: " << (int)recvData[recvLen - 1]);
			return false;
		}
		std::vector<uint8_t>    packet_out;
		packet_out.reserve(recvLen);
		AntonymData(recvData, recvLen,packet_out);  //��ת��
		uint8_t p_verify = CalculateXORChecksum(packet_out,1,2);  //�����ֵʱ ȥ����βһ���ֽ� �� β��ǰһ���ֽ�(���ֵ����ռ�õ��ֽ�)
		int pcketoutLen = packet_out.size();
		if (p_verify != packet_out[pcketoutLen -2])  //У��Ա� 
		{
			LogError("Calculate xorsum :" << (int)p_verify << " is different from recv xorsum: " << (int)recvData[recvLen - 2]);
			return false;
		}

		std::vector<uint8_t> fsuid(20,0);
		for (int i = 0; i < 20; ++i)
			fsuid[i] = packet_out[i + 1];
			//fsuid[i] = packet_out[i + 9];
		std::string strFsuID(fsuid.begin(), fsuid.end());
		/*if (CMMConfig::instance()->GetFsuId().compare(strFsuID) != 0)
		{
			LogError("config fsuid :" << CMMConfig::instance()->GetFsuId().c_str() << " is different from recv fsuid: " << strFsuID.c_str());
			return false;
		}*/
		int16_t xmlLen = (static_cast<uint16_t>(packet_out[37]) << 8) | static_cast<uint16_t>(packet_out[36]);
		
		for (int16_t i = 0; i < xmlLen; ++i)
			outData.push_back(packet_out[38 + i]);	
		LogInfo("recv data unpack size:" << xmlLen << " outData:"<< outData.size());
		return true;
	}

}

