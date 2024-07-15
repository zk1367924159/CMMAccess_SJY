#include <fcntl.h>
#include <unistd.h>
#include <Poco/URI.h>
#include <Poco/Tuple.h>
#include <Poco/JSON/Object.h>
#include <Poco/SingletonHolder.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPClientSession.h>

#include "CLog.h"
#include "Data.h"
#include "../../ExtAppIpc/ExtAppIpcApi.h"
#include "CMMUart.h"
#include "CMMAccess.h"
#include "CMMConfig.h"

#define BUFFER_SIZE             1024

#define REG_MAX_NUM             65536
namespace CMM_SJY
{
    enum
    {
        E_DOOR_OPEN = 0,
        E_DOOR_CLOSE = 1,
    };

    std::string toString(const uint8_t* buffer, const int& length)
    {
        static constexpr char hexmap[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                             '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        std::string hexString;

        for (int index = 0; index < length; index++)
        {
            hexString += hexmap[(buffer[index] >> 4) & 0x0F];
            hexString += hexmap[(buffer[index] >> 0) & 0x0F];
        }

        return hexString;
    }


    CMMUart::CMMUart() : m_buffer(new uint8_t[BUFFER_SIZE])
    {
        m_running = false;
        m_uartname = CMMConfig::instance()->GetParam(param::UartName, "BottomBoard_Uart6").c_str();
        m_baudrate = CMMConfig::instance()->GetParam(param::BaudRate, "9600").convertInt();
        m_dataBits = CMMConfig::instance()->GetParam(param::DataBit, "8").convertInt();
        m_parity = CMMConfig::instance()->GetParam(param::Parity, "N").c_str();
        m_stopBits = CMMConfig::instance()->GetParam(param::StopBit, "1").convertInt();
        m_slaveID = CMMConfig::instance()->GetParam(param::SlaveID, "1").convertInt();
    }

    CMMUart::~CMMUart()
    {

    }

    void CMMUart::Start()
    {
        if (!m_running)
        {
            m_running = true;

            m_thread = new Poco::Thread;
            m_thread->setName("CMMUart");
            m_thread->start(*this);
        }
    }

    void CMMUart::Stop()
    {
        if (m_running)
        {
            m_running = false;
        }
    }

    void CMMUart::run()
    {
        int     length = 0;

        while (m_running)
        {
            Poco::SharedPtr<char>   recvBuffer;

            length = APPAPI::ReadUart(m_uartname, recvBuffer, 500);
            if (length > 0)
            {
                uint8_t* bufferPtr = reinterpret_cast<uint8_t*>(recvBuffer.get());
                LogInfo("read uart buffer: " << toString(bufferPtr, length));
                requestHandler(bufferPtr, length);
            }
            else
            {
                usleep(100 * 1000);
            }
        }
    }

    void CMMUart::setSlaveID(const int& slaveID)
    {
        std::lock_guard<std::mutex> lockGuard(m_rwLock);
        m_slaveID = slaveID;
    }

    void CMMUart::setBaudrate(const int& baudrate)
    {
        std::lock_guard<std::mutex> lockGuard(m_rwLock);
        if (m_baudrate != baudrate)
        {
            m_baudrate = baudrate;
            setUartParam();
        }
    }

    void CMMUart::setDataBits(const int& dataBits)
    {
        std::lock_guard<std::mutex> lockGuard(m_rwLock);
        if (m_dataBits != dataBits)
        {
            m_dataBits = dataBits;
            setUartParam();
        }
    }

    void CMMUart::setStopBits(const int& stopBits)
    {
        std::lock_guard<std::mutex> lockGuard(m_rwLock);
        if (m_stopBits != stopBits)
        {
            m_stopBits = stopBits;
            setUartParam();
        }
    }

    void CMMUart::setParity(const std::string& parity)
    {
        std::lock_guard<std::mutex> lockGuard(m_rwLock);
        if (m_parity != parity)
        {
            m_parity = parity;
            setUartParam();
        }
    }

    int CMMUart::getUartID()
    {
        int uartID = 1;
        if (!m_uartname.empty())
        {
            std::string numberStr;
            // 从字符串末尾向前读取数字字符
            for (int i = m_uartname.length() - 1; i >= 0; --i)
            {
                if (std::isdigit(m_uartname[i]))
                {
                    numberStr = m_uartname[i] + numberStr; // 将数字字符添加到前缀
                }
                else
                {
                    break; // 遇到非数字字符，停止读取
                }

                // 如果读取的数字字符超过2位，停止读取
                if (numberStr.length() > 2)
                {
                    break;
                }
            }

            if (!numberStr.empty())
            {
                uartID = std::stoi(numberStr)-1; // 将字符串转换为整数
            }
        }
        return uartID;
    }

    void CMMUart::setUartName(const std::string& uartname)
    {
        std::lock_guard<std::mutex> lockGuard(m_rwLock);

        if (uartname != m_uartname)
        {
            m_uartname = uartname;
            setUartParam();
        }
    }

    void CMMUart::setUartParam()
    {
        APPAPI::SetUartParam(m_uartname, m_baudrate, m_dataBits, m_parity, m_stopBits);

        LogInfo("串口 " << m_uartname << " 设置参数: 波特率 - " << m_baudrate << ", 数据位 - " << m_dataBits << ", 校验 - " << m_parity << ", 停止位 - " << m_stopBits);
    }

    void CMMUart::requestHandler(uint8_t* recvBuffer,int length)
    {
        std::vector<uint8_t> buffer;
        buffer.reserve(length);
        for (int i=0;i<length;++i)
        {
            buffer.push_back(recvBuffer[i]);
        }
        if(!CMMAccess::instance()->SendUartDataToSC(buffer))
            LogError("SendUartDataToSC failed.");
    }

    bool CMMUart::writeData(std::vector<uint8_t>& uartData)
    {
        LogInfo("write uart buffer: " << toString(uartData.data(), uartData.size()));
        int nRet = APPAPI::WriteUart(m_uartname, (char*)uartData.data(), uartData.size(), 100);
        if (nRet < 0)
        {
            LogInfo("write buffer error : " << nRet);
            return false;
        }
        return true;
    }

}
