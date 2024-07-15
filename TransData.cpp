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

	// 定义一个函数来处理转义  
	void CTransData::EscapeData(std::vector<uint8_t>::const_iterator start, std::vector<uint8_t>::const_iterator end ,std::vector<uint8_t>& packet)
	{
		while (start != end) 
		{
			switch (*start)
			{
			case 0xFF:
				// 假设转义规则是：遇到0xFF，先加转义字符0x7D，然后0xFF变为0xFF+1=0x00  
				packet.push_back(0xFD);
				packet.push_back(0x00);
				break;
			case 0xFE:
				// 假设遇到0xFE，先加转义字符0x7D，然后0xFE变为0xFE+1=0xFF（注意这里可能会与0xFF的转义冲突，具体取决于协议）  
				// 或者，如果协议有不同的规则，比如直接变为另一个固定值或进行其他变换  
				packet.push_back(0xFD);
				packet.push_back(0xFF); // 假设直接变为0xFF，具体取决于协议  
				break;
			case 0xFD:
				// 类似地处理0xFD  
				packet.push_back(0xFD);
				packet.push_back(0x02); // 这里只是示例，实际值取决于协议  
				break;
			default:
				// 其他字节直接添加  
				packet.push_back(*start);
			}
			++start;
		}
	}

	// 定义一个函数来处理反转义  
	void CTransData::AntonymData(const std::vector<uint8_t>& data, int recvLen ,std::vector<uint8_t>& packet)
	{
		int i = 0;
		while (i < recvLen)
		{
			if (data[i] == 0xFD)  // 反转义字符开始  
			{
				if (i + 1 >= recvLen)
				{
					throw std::runtime_error("Incomplete escape sequence in data");
				}

				switch (data[i + 1]) 
				{
				case 0x00: // 还原0xFF  
					packet.push_back(0xFF);
					i += 2; // 跳过转义序列  
					break;
				case 0x01: // 还原0xFE（假设这是协议规定的）  
					packet.push_back(0xFE);
					i += 2; // 跳过转义序列  
					break;
				case 0x02: // 还原0xFD（这里使用0x02作为示例，具体值取决于协议）  
					packet.push_back(0xFD);
					i += 2; // 跳过转义序列  
					break;
				default:
					throw std::runtime_error("Invalid escape sequence in data");
				}
			}
			else 
			{
				// 不是转义字符，直接添加到结果中  
				packet.push_back(data[i]);
				i++;
			}
		}
	}

	// 计算异或校验值  
	uint8_t CTransData::CalculateXORChecksum(const std::vector<uint8_t>& data, size_t headerLen, size_t tailLen) {
		uint8_t checksum = 0;
		size_t dataSize = data.size();
		if (dataSize >= headerLen + tailLen) 
		{
			// 对除了包头和包尾之外的数据进行异或操作  
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
			packet.push_back(0x00);          //sc地址
		
		CData fsuId = CMMConfig::instance()->GetFsuId();
		if (uartData.size() <= 0 || fsuId.size() <= 0)
		{
			LogError("sendData length is so short : "<< uartData.size() <<"  or fsuid is null.");
			throw std::runtime_error("An error occurred while getting data");
		}
			
		// 将fsuid的内容复制到vector中  
		for (int i = 0; i < 20 ; ++i)
		{
			if(i < fsuId.size())
				packet.push_back(static_cast<uint8_t>(fsuId[i]));   //fsuid
			else
				packet.push_back(0x00);   //fsuid
		}

		packet.push_back(0x01);    //子设备类型
		uint8_t p_uart = ((CMMAccess::instance()->m_uartService->getUartID() & 0xf) << 4) | (CMMAccess::instance()->m_uartService->m_slaveID & 0xf);
		LogInfo("p_uart: " << (int)p_uart);
		packet.push_back(p_uart);    //串口号+地址号

		uint16_t p_len = 5 + uartData.size();
		
		packet.push_back(p_len & 0xff);  //协议族数据包长度
		packet.push_back((p_len >> 8) & 0xff);

		packet.push_back(0x00);   //应答类型
		uint16_t command_id = 0x0001;
		
		packet.push_back(command_id & 0xff);  //命令编号
		packet.push_back((command_id >> 8) & 0xff);

		uint16_t t_len = static_cast<uint16_t>(uartData.size());
		packet.push_back(t_len & 0xff); //xmldata数据长度
		packet.push_back((t_len >> 8) & 0xff);

		for (uint16_t i = 0; i < t_len ; ++i)
		{
			packet.push_back(uartData[i]);   //xmldata
		}
		uint8_t p_verify = CalculateXORChecksum(packet, 1 ,0);
		packet.push_back(p_verify);   //校验
		packet.push_back(0xFE);   //结尾

		std::vector<uint8_t>  packet_out;
		packet_out.push_back(0xFF);
		EscapeData(packet.begin() + 1, packet.end()-1, packet_out); ///转义
		packet_out.push_back(0xFE);   //结尾
		return packet_out;
	}

	std::vector<uint8_t> CTransData::PackageSendHeart()
	{
		int nMaxPackageSize = 38;
		std::vector<uint8_t>    packet;
		packet.reserve(nMaxPackageSize);
		packet.push_back(0xFF);
		for (int i = 0; i < 8; ++i)
			packet.push_back(0x00);          //sc地址

		CData fsuId = CMMConfig::instance()->GetFsuId();
		if (fsuId.size() <= 0)
		{
			LogError("fsuid is null.");
			throw std::runtime_error("An error occurred while getting data");
		}

		// 将fsuid的内容复制到vector中  
		for (int i = 0; i < 20; ++i)
		{
			if (i < fsuId.size())
				packet.push_back(static_cast<uint8_t>(fsuId[i]));   //fsuid
			else
				packet.push_back(0x00);   //fsuid
		}

		packet.push_back(0x01);    //子设备类型
		uint8_t p_uart = ((CMMAccess::instance()->m_uartService->getUartID() & 0xf) << 4) | (CMMAccess::instance()->m_uartService->m_slaveID & 0xf);
		LogInfo("p_uart: " << (int)p_uart);
		packet.push_back(p_uart);    //串口号+地址号

		uint16_t p_len = 3;
		//packet.push_back((p_len >> 8) & 0xff);
		//packet.push_back(p_len & 0xff);  //协议族数据包长度
		packet.push_back(p_len & 0xff);  //协议族数据包长度
		packet.push_back((p_len >> 8) & 0xff);
		

		packet.push_back(0xED);   //RtnFlag
		uint16_t command_id = 0x0002;
		
		packet.push_back(command_id & 0xff);  //命令编号
		packet.push_back((command_id >> 8) & 0xff);

		uint8_t p_verify = CalculateXORChecksum(packet, 1, 0);
		packet.push_back(p_verify);   //校验
		packet.push_back(0xFE);   //结尾

		std::vector<uint8_t>  packet_out;
		packet_out.push_back(0xFF);
		EscapeData(packet.begin()+1 ,packet.end()-1, packet_out); ///转义
		packet_out.push_back(0xFE);   //结尾
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
		if (p_header != 0xFF || recvData[recvLen - 1] != 0xFE)  //判断是否有开始和结束标识
		{
			LogError("p_header is not 0xff :" << (int)p_header << " or p_end is not 0xFE: " << (int)recvData[recvLen - 1]);
			return false;
		}
		std::vector<uint8_t>    packet_out;
		packet_out.reserve(recvLen);
		AntonymData(recvData, recvLen,packet_out);  //反转义
		uint8_t p_verify = CalculateXORChecksum(packet_out,1,2);  //算异或值时 去除首尾一个字节 和 尾部前一个字节(异或值本身占用的字节)
		int pcketoutLen = packet_out.size();
		if (p_verify != packet_out[pcketoutLen -2])  //校验对比 
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

