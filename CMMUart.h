#ifndef CMMUart_H
#define CMMUart_H

#include <stdint.h>

#include <mutex>
#include <memory>
#include <string>

#include <Poco/Clock.h>
#include <Poco/Event.h>
#include <Poco/Tuple.h>
#include <Poco/Thread.h>

using writeData = std::vector<uint8_t>;

namespace CMM_SJY
{
    class CMMUart : public Poco::Runnable
    {
    public:
        CMMUart();
        virtual ~CMMUart();



        virtual void run();
        void Start();
        void Stop();
        void setSlaveID(const int& slaveID);
        void setBaudrate(const int& baudrate);
        void setDataBits(const int& dataBits);
        void setStopBits(const int& stopBits);
        void setParity(const std::string& parity);
        void setUartName(const std::string& uartname);
        int  getUartID();

        bool writeData(std::vector<uint8_t>& uartData);
    protected:
        virtual void setUartParam();
        virtual void requestHandler(uint8_t* recvBuffer, int length);
    public:
        uint8_t                         m_slaveID;
        int32_t                         m_baudrate;
        int32_t                         m_dataBits;
        int32_t                         m_stopBits;
        std::string                     m_parity;
        std::string                     m_uartname;
        
    protected:
        bool                            m_running;

        
        std::mutex                      m_rwLock;
        std::shared_ptr<uint8_t>        m_buffer;
        Poco::SharedPtr<Poco::Thread>   m_thread;
    };
}
#endif // CMMUart_H
