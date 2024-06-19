#ifndef _CMMCONFIG_H
#define _CMMCONFIG_H
#include "Data.h"
#include "NetComm/CXmlElement.h"
#include "CMMCommonStruct.h"
#include "SmartLock.h"




namespace CMM_SJY{
	class CMMConfig{
	public:
		CData m_dbFile;
		CData m_fsuId;
		CData m_userName;
		CData m_password;
		CData m_RoomID;
		CData m_RoomName;
		CData m_SiteID;
		CData m_SiteName;
		CData m_DevCfgFileName;
		static CMMConfig *_instance;
		ISFIT::CXmlDoc m_doc;
		std::map<CData, TDevConf> m_devCfg;
		std::map<CData, CData> m_dictionary;  //编号 -- 名称
		CData m_ftpUsr;
		CData m_ftpPasswd;
		CData m_fsuIp;
		CData m_fsuPort;
		CData m_scIp;
		CData m_scPort;
		CData m_scIpRoute;
		CData m_fsuConfigTime;
	
		CData m_heartbeatTimeout;

		CData m_loginTimeout;

		CData m_getMeasureMentTime; //获取监控点性能数据文件间隔时间 /单位 分钟

		CData m_level1DlyTime;
		CData m_level2DlyTime;
		CData m_level3DlyTime;
		CData m_level4DlyTime;
		CData m_fsuVersion;
		bool m_bUpdate;
		
		CData m_IgnoreAlarmLevel;
		std::vector<int> m_IgnoreAlarmLevelVec;
	    std::list <CData> m_devIdList;
		std::map <CData,int> m_dev2MeterList;  //每个dev下 量的个数
		ISFIT::CSmartMutex m_devCfgMutex;
		std::map<CData, std::list<CData>> m_familyIPList;
	public:
		static CMMConfig* instance();
		int Init();
		void ReadCMMConfigData();
		CData GetParam(CData key, CData defVal);
		int SetParam(CData key, CData val);
		void SetIgnoreAlarmLevel(CData val);
		std::vector<int>& GetIgnoreAlarmLevel(){ return m_IgnoreAlarmLevelVec;}
		std::vector<int> vStringSplit(const CData& s, const std::string& delim=",");
		CData NMAlarmID(CData signalId);
		CData GetFsuId();
		void SetFsuId(CData fsuId);
		void SetFsuPort(CData port);
		CData GetFsuPort();
		CData GetFsuIp();
		void SetFsuIp(CData ip);
		CData GetUserName();
		void SetUserName(CData userName,bool saveDb=false);
		CData GetPassword();
		void SetPassword(CData password,bool saveDb=false);
		int SetFtpPasswd( CData usr , bool saveDb=false);
		int SetFtpUsr(CData usr, bool saveDb=false);
		CData GetFsuConfigTime();
		void SetFsuConfigTime( CData time );
		CData GetSiteID();
		CData GetSiteName();
		void SetSiteID(CData SiteID,bool saveDb=false);
		void SetSiteName( CData SiteName ,bool saveDb=false);
		CData GetRoomID();
		CData GetRoomName();
		void SetRoomID(CData RoomID,bool saveDb=false);
		void SetRoomName( CData RoomName,bool saveDb=false);
		std::map<CData, TDevConf> &GetDevices();
		int GetDevConf(CData devid, TDevConf& cfg);
		int SetSemaphoreConf(CData devid, TSemaphore& cfg);
		void GetSemaphoreConf(std::map<CData, std::list<TSemaphore>>& reqDevMap);
		int GetSemaphoreConf(CData devid, TSemaphore& cfg);
		int SetThresholdConf(CData devid, TThreshold& cfg);
		int SetStorageRuleConf( CData devid, TSignal& cfg );
		int SetDevCfg(std::map<CData, TDevConf>& devMap, std::list<CData>& scucessList, std::list<CData>& failList);
		int GetDevMetes(TDevConf &cfg);
		void SaveFile();
		void UpdateCfgFile();
		bool OnUpdateCfgFileTimer();
		void addAcceptIP(CData familyType, std::list<CData>& IPList);
		bool isAcceptIp(CData familyType, CData ip);
		CData GetDictionaryName(CData id);
		CData CreateMeasurefile(CData& timestamp);
		bool WriteMeasurefile();
		int SetMeteValues(std::map<CData, CData>& param, TSemaphore& semaphore, int nType);
		int SetMeteStorageRule(std::map<CData, CData>& param, TSignal& Signal, int nType);
		int SetMeteThreshold(std::map<CData, CData>& param, TThreshold& theshold, int nType);
	private:
		void CreateConfigFile();
		void ReadDevCfgFromObj(std::list <CData>& devIdList);
		int GetDev(CData devId, TDevConf& cfg);
	
	};
}
#endif
