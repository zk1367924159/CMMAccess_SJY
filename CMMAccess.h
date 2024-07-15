#ifndef _CMMACCESS_H
#define _CMMACCESS_H
#include "Poco/Mutex.h"
#include "Poco/Thread.h"
#include "stdsoap2.h"
#include "soapStub.h"
#include "Poco/Timestamp.h"
#include "Data.h"
#include "CMMMsgProcess.h"
#include "CMMServer.h"
#include "NetComm/CmdController.h"
#include "CMMLogFile.h"
#include "NetComm/ModuleVersions.h"

#include "Poco/Runnable.h"
#include "Poco/SharedPtr.h"
#include "Poco/Exception.h"
#include "Poco/RefCountedObject.h"

#include <functional>
#include "ModuleManage.h"
#include "CMMDataLog.h"

#include "Timer.h"
#include "HttpClient.h"
#include "HttpServer.h"
#include "WebServer.h"
#include "UdpClient.h"
#include "UdpServer.h"
#include "CMMUart.h"

using namespace Poco;
using namespace ISFIT;
using namespace CMM_SJY;

namespace CMM_SJY {
	enum
	{
		CMM_REGISTER_FAILED =0,
		CMM_REGISTER_SUCCESS = 1,
	};
	void threadFunction2(void* arg);
	class  CMMAccess:public Poco::Runnable
	{
		public:
			static CMMAccess* instance();
			CData GetNetIfcParam(CData ifconfig, CData key);
			void AddRoute(CData destIp, CData gateWay);
			int AddLinuxSysUser(CData user, CData passwd, CData dir);
			int DelLinuxSysUser(CData user);
			int ModifyLinuxSysPasswd(CData user, CData passwd);
			void SaveDataLog(std::map<CData, CData>& msg);
			CData GetIfcIp(CData ifcName);
			CData GetLocalMac() ;
			
			virtual void run();
			void runEx();
			void OnHeartBeat();
			void UpdateInterval(CData interval);
			int DoMsgProcess(char* request, char* response, int size);
		    int DoMsgProcessError(char* request, char* response, int size, int errorType);
		    int GetServerStatus(TServerStatus& sts);
		    int SendRequestToServer(CData& reportInfo);
		    static void LogSoapError(struct soap* soap);
		    void TestStart(int arg);
		    void NotifyAlarm(std::map<CData, CData>& msg);
		    void NotifySendData(std::map<CData, std::list<TSemaphore> >& mapSem);
		    void SetAlarmDlyTime(CData dlyTime1, CData clearDlyTime1,
			CData dlyTime2, CData clearDlyTime2,
			CData dlyTime3, CData clearDlyTime3);
		int UpdateParam(std::map<CData, CData>& paramMap, std::map<CData, CData>& errorMap);
		void initialize(std::list<std::tuple<CData, CData> >& param);
		CData describe() { return "中国移动B接口APP_V1.0.0.20240713"; };	// DAHAI
		void start();
		void stop();
		void unInitialize();
		bool checkIpAuth(const char* http_ip, int familyType);
		bool checkAuth(std::string soapXmlData);
		void UpdateAuthHeader(const CData& message, CData& authHeader, CData& strtoken);
		void SetUdpLoginState(bool isLoginOk);
			void ReportDevConf();
			void ReportData(std::map<CData, std::list<TSemaphore>>& mapSem);
			bool SendUartDataToSC(std::vector<uint8_t>& sendBuffer);
			bool writeDataToUart(std::vector<uint8_t>& sendBuffer);
		CData GetSoapXmlData(const char* soapXml, std::string httpBegin, std::string httpEnd);
		void disPlayText(const char* pstr);
		bool GetLoginState();
		
		private:
			CMMAccess();
			void Init();
			void DeInit();
			void Login();
			void UpdateModuleInfo();
		    void Test();
			void SetLoginState(bool isLoginOk);
			void ReportAlarms();
			void SetPowerdownAlarmParam(int arg);
			void UpdateDevConf(int arg);
			void WriteMeasureFile(int arg);
			int FromAlarmInfoToTAlarm2(std::map<CData, CData>& msg, TAlarm& alarm);
			CData resolveDomainToIp(const char* domainName);
			
		public:
			CData m_hashPassword;   //sh256散列后的密码
			bool m_bLoginOK;
			bool m_bIsUart;        //是否开启透传模式
			bool m_bStart;
			CData m_scUdpPoint;//SC udp
			int m_registerTime;
			int m_heartBeatTime;
			Poco::Timestamp m_lastMsgTimeBak;
			bool m_udpRegisterStatus;
			Poco::SharedPtr<CUdpClient>   m_udpClient;
			Poco::SharedPtr<CUdpServer>   m_udpServer;
			Poco::SharedPtr<CMMUart>   m_uartService;
		private:
			static Poco::FastMutex m_mutex;
			Poco::Thread m_thread;
			Poco::Thread m_secondThread;
			static CMMAccess* _instance;
			Poco::Timestamp m_lastMsgTime;
			int m_registerStatus;
			int m_RightLevel;
			int m_wirteFileTime;
			int m_nRetry;  //重试次数
			CData m_scEndPoint;  //SC http
			CData m_fsuEndPoint;
			CData m_scDomain; //sc域名
			MsgProcess m_msgProcess;		
			//int m_heartBeatTimeout;	
			Poco::SharedPtr<CHttpClient>   m_client;
			Poco::SharedPtr<CHttpServer>   m_server;
			Poco::SharedPtr<CWebServer>   m_webServer;
			Poco::FastMutex	m_checkMutex;
			Poco::FastMutex m_alarmMutex;
			std::list<TAlarm> m_alarmList;
			Poco::FastMutex m_alarmMutexBak;
			std::list<TAlarm> m_alarmListBak;
			CMMLogFile m_log;
			CMMDataLog m_datalog;
			Poco::SharedPtr<ISFIT::CTimer> m_recoverPowerdownAlarmParamTimer;
			Poco::SharedPtr<ISFIT::CTimer> m_updateDevTimer;
			Poco::SharedPtr<ISFIT::CTimer> m_wirteMeasurementFileTimer;
	};

};
extern "C" CIModule* GetModule();
#endif
