//canyon 2019 0902

#include <iomanip>  
#include <fstream>  
#include <algorithm>  
#include "CMMConfig.h"
#include "CMMDeviceConfig.h"
#include "CLog.h"
#include "CMMMeteTranslate.h"
#include "CMMProtocolEncode.h"
#include "CMMProtocolDecode.h"
#include "SysCommon.h"
#include "CMMCommonStruct.h"
#include "CMMAccess.h"
#include "Poco/SharedPtr.h"
#include "Poco/DateTimeFormatter.h"  
#include "Poco/DateTime.h" 
#include "../../Common/ShareFile.h"
#include "../../ExtAppIpc/ExtAppIpcApi.h"
#include "../../ExtAppIpc/ExtAppIpcStruct.h"

#define  CMM_DEVICE_CONFIG  "/Config/devices"
//#define  CMM_DEVICE_CONFIG  "/userdata/Config/devices"
#define  CMM_DEVICE_CONFIG_FILE_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Devices></Devices>"
#define  CMM_TEMPLATES_XML "/appdata/config/templates.xml"
namespace CMM_SJY{
	CMMConfig * CMMConfig::_instance = NULL;

	CMMConfig* CMMConfig::instance()
	{
		if(_instance == NULL)
		{
			_instance = new CMMConfig();
		}
		return _instance;
	}


	std::vector<int> CMMConfig::vStringSplit(const  CData& s, const std::string& delim)
	{
	  //  std::vector<std::string> elems;
		std::vector<int> elems;
		size_t pos = 0;
		size_t len = s.length();
		size_t delim_len = delim.length();
		if (delim_len == 0) return elems;
		while (pos < len)
		{
			int find_pos = s.find(delim, pos);
			if (find_pos < 0)
			{
				elems.push_back(s.substr(pos, len - pos).convertInt());
				break;
			}
			elems.push_back(s.substr(pos, find_pos - pos).convertInt());
			pos = find_pos + delim_len;
		}
		return elems;
	}
	
	void CMMConfig::SetIgnoreAlarmLevel(CData val)
	{
		m_IgnoreAlarmLevel = val;
		m_IgnoreAlarmLevelVec.clear();
		if(m_IgnoreAlarmLevel.length()>0)
		{
			m_IgnoreAlarmLevelVec=vStringSplit(m_IgnoreAlarmLevel);
		}
	}
	
	CData CMMConfig::GetParam(CData key, CData defVal)
	{
		CData val = APPAPI::GetExtAppParam(key);
		if (val.size()>0) return val;
		return defVal;
	}

	int CMMConfig::SetParam(CData key, CData val)
	{
		return APPAPI::SaveExtAppParam(key, val);
	}

	int CMMConfig::Init()
	{

		m_fsuId = GetParam(CMM_SJY::param::FsuId, "");
		//m_FsuCode = GetParam(CTower::param::FsuCode, "");
		m_fsuIp = GetParam(CMM_SJY::param::FsuIp, "");
		m_fsuPort = GetParam(CMM_SJY::param::FsuPort, "");
		m_fsuConfigTime = GetParam(CMM_SJY::param::DevCfgTime, "");

		m_userName = GetParam(CMM_SJY::param::UserName, "");
		m_password = GetParam(CMM_SJY::param::Password, "");
		m_ftpUsr = GetParam(CMM_SJY::param::FtpUsr, "");
		m_ftpPasswd = GetParam(CMM_SJY::param::FtpPasswd, "");
	
		m_scIp =  GetParam(CMM_SJY::param::SCIp, "");
		m_scPort =  GetParam(CMM_SJY::param::SCPort, "");
 		m_scUdpIp = GetParam(CMM_SJY::param::SCUdpIp, "");
		m_scUdpPort = GetParam(CMM_SJY::param::SCUdpPort, "");
		m_scIpRoute =  GetParam(CMM_SJY::param::SCIpRoute, "");

		m_SiteID = GetParam(CMM_SJY::param::SiteID, "");
		m_SiteName = GetParam(CMM_SJY::param::SiteName, "");
		m_RoomID = GetParam(CMM_SJY::param::RoomID, "");
		m_RoomName = GetParam(CMM_SJY::param::RoomName, "");

		
		m_IgnoreAlarmLevel = GetParam(CMM_SJY::param::IgnoreAlarmLevel, "");
		m_IgnoreAlarmLevelVec.clear();
		if(m_IgnoreAlarmLevel.length()>0)
		{
			m_IgnoreAlarmLevelVec=vStringSplit(m_IgnoreAlarmLevel);
		}
		m_fsuVersion = GetParam(CMM_SJY::param::SoftVer, "");
		CMMAccess::instance()->AddLinuxSysUser(m_ftpUsr, "", "/");
		CMMAccess::instance()->ModifyLinuxSysPasswd(m_ftpUsr, m_ftpPasswd);
		//读取移动动环+告警相关配置文件
		ReadCMMConfigData();
		//m_fsuId =  ISFIT::Config::getString(CMM_SJY::param::FsuId, "MSJW201605170001");
		m_DevCfgFileName = CMM_DEVICE_CONFIG; 
		m_DevCfgFileName += "_"+m_fsuId + ".xml";
		LogInfo("dev cfg file name"<<m_DevCfgFileName.c_str());

		//启用或重启都应该重新读列表
		CreateConfigFile();		
		/*LogInfo("++++++++++++++++++++++++++++++++++");
		ISFIT::ShareFile::printMeterFileInfo("");
		LogInfo("++++++++++++++++++++++++++++++++++");*/
		return 0;
	}


	std::vector<std::string> extractValues(const std::string& input) 
	{
		std::vector<std::string> values;
		std::istringstream iss(input);
		std::string value;

		while (std::getline(iss, value, ',')) 
		{
			// 移除可能的空白字符（如果有的话）  
			value.erase(std::remove_if(value.begin(), value.end(), isspace), value.end());
			values.push_back(value);
		}

		return values;
	}

	// 使用初始化列表的示例  
	bool areVectorsEqualSize(std::initializer_list<const std::vector<std::string>*> vecs) 
	{
		if (vecs.size() == 0) return true; // 空列表，认为所有vector大小相等  

		size_t size = (*vecs.begin())->size(); // 假设第一个vector非空  
		for (const auto* vec : vecs) {
			if (!vec || vec->size() != size) {
				return false;
			}
		}
		return true;
	}

	void CMMConfig::ReadCMMConfigData()
	{
		ISFIT::CXmlDoc doc;
		if (doc.ParseFile(CMM_TEMPLATES_XML) < 0)
		{
			LogError("ParseFile error .");
			return;
		}
		ISFIT::CXmlElement templates = doc.GetElementByName("templates");
		if (templates == NULL)
		{
			return;
		}
		{
			int nIndex = 0;
			ISFIT::CXmlElement temElement = templates.GetSubElement("template", nIndex++);
			while (temElement != NULL)
			{
				
				CData strPlatFrom = temElement.GetAttribute("platform");
			/*	LogInfo("strPlatFrom : " << strPlatFrom.c_str());*/
				if (strPlatFrom.compare("cmm") == 0)
				//if (strPlatFrom.compare("cmm") == 0)
				{
					ISFIT::CXmlElement metes = temElement.GetSubElement("metes");
					if (metes != NULL)
					{
						int nIsubIndex = 0;
						ISFIT::CXmlElement mete = metes.GetSubElement("mete", nIsubIndex++);
						while (mete != NULL)
						{
							CData channel = mete.GetAttribute("channel");
							CData name = mete.GetAttribute("name");
							//LogInfo("channel : " << channel.c_str() << " name:"<< name.c_str());
							m_dictionary[channel] = name;
							mete = metes.GetSubElement("mete", nIsubIndex++);
						}
					}
				}
				temElement = templates.GetSubElement("template", nIndex++);
			}
		}
		LogInfo("read dictionary size: " << m_dictionary.size());
	}

	CData CMMConfig::GetDictionaryName(CData id)
	{
		auto it = m_dictionary.find(id);
		if (it == m_dictionary.end())
		{
			return NULL;
		}
		return it->second;
	}

	void CMMConfig::CreateConfigFile()
	{
		UpdateCfgFile();
	}

	CData CMMConfig::NMAlarmID(CData signalId)
	{
		CData prefix = "60100100";
		prefix += signalId;
		return prefix;
	}
	
	std::map<CData, TDevConf> &CMMConfig::GetDevices()
	{
		//防止另外一个地方在修改m_devCfg
		//FastMutex::ScopedLock lock(m_devCfgMutex);//
		return m_devCfg;
	}

	bool  CMMConfig::OnUpdateCfgFileTimer()
	{
		//LogInfo("--------OnUpdateCfgFileTimer："<<m_DevCfgFileName.c_str());
		std::list <CData> devIdList;
		std::map <CData, int> dev2MeterList;
		APPAPI::GetDevId("alias", devIdList);

		int curSize=devIdList.size();
		int size=m_devIdList.size();
		bool bUpdate=false;
		std::list<CData>::iterator its = devIdList.begin();
		while (its != devIdList.end())
		{
			CData devID = *its;
			std::set<CData> attrSet;
			attrSet.insert("meterId");
			std::list<std::map<CData, CData> > infoList;
			APPAPI::GetMeterInfo(devID, "alias", attrSet, infoList);
			dev2MeterList[devID] = infoList.size();
			its++;
		}
		
		
		if(curSize!=size)
		{
			bUpdate=true;
		}
		else
		{
			//比对内容是否一样
			std::list<CData>::iterator it=devIdList.begin();
			while(it!=devIdList.end())
			{
				CData devID = *it;
				if(std::find(m_devIdList.begin(),m_devIdList.end(), devID.c_str()) == m_devIdList.end())//没查找到，说明有更新
				{
					bUpdate=true;
					break;
				}
				else          //查到 但是再检测 量数量 是否一致
				{
					int nowSize = dev2MeterList[devID];
					int beforeSize = m_dev2MeterList[devID];
					if (nowSize != beforeSize)
					{
						LogInfo("--before size:"<< beforeSize << "  --now size:"<< nowSize);
						bUpdate = true;
						break;
					}
					
				}
				it++;
			}
			
		}

		if(bUpdate)
		{
			ISFIT::SmartLock lock(m_devCfgMutex);
			ReadDevCfgFromObj(devIdList);
			SaveFile();	
			m_devIdList=devIdList;	
			m_dev2MeterList = dev2MeterList;
			LogInfo("--------go to update-------");
		}
		return bUpdate;
	}
	
	void CMMConfig::UpdateCfgFile()
	{
		//LogInfo("--------UpdateCfgFile："<<m_DevCfgFileName.c_str());
		std::list <CData> devIdList;
		std::map <CData, int> dev2MeterList;

		APPAPI::GetDevId("alias", devIdList);
		static bool bFirst=true;
		if(bFirst)
		{
			bFirst=false;
			m_devIdList=devIdList;
			
			std::list<CData>::iterator it = devIdList.begin();
			while (it != devIdList.end())
			{
				CData devID = *it;
				std::set<CData> attrSet;
				attrSet.insert("meterId");
				std::list<std::map<CData, CData> > infoList;
				APPAPI::GetMeterInfo(devID, "alias", attrSet, infoList, 10000);
				dev2MeterList[devID] = infoList.size();
				it++;
			}
			m_dev2MeterList = dev2MeterList;
		}
		
		ISFIT::SmartLock lock(m_devCfgMutex);
		ReadDevCfgFromObj(devIdList);
		SaveFile();
		m_bUpdate=true;
		m_bUpdateBak = true;
	}

	//canyon
	void CMMConfig::ReadDevCfgFromObj(std::list <CData>& devIdList)
	{
		m_devCfg.clear();
		for (auto it=devIdList.begin(); it!=devIdList.end(); it++)
		{
			CData aliasDevId = *it;
			if (aliasDevId.length() < 4)
				continue;
			

			std::map<CData,CData> paramMap;
			APPAPI::GetDevParam(aliasDevId,"alias", paramMap,5000);
				
			CData devId = paramMap["devId"];
			CData deviceName = paramMap["aliasDevName"];
			TDevConf dev={0};
			dev.DeviceID =aliasDevId;
			dev.DeviceName = deviceName;
			dev.DeviceType = aliasDevId.substr(0,2);
			dev.DeviceSubType = aliasDevId.substr(2,2);
			LogInfo("--------aliasDevId :" << aliasDevId << " deviceID:" << devId);
			auto iter = CMMDeviceConfig::instance()->GetDevices().find(devId);
			if (iter != CMMDeviceConfig::instance()->GetDevices().end())
			{
				TDeviceInfo& sinfo = iter->second;
				dev.Brand = sinfo.Brand;
				dev.Model = sinfo.Model;
				dev.DevDescribe = sinfo.Desc;
				dev.Version = sinfo.Version;
				dev.RatedCapacity = sinfo.RatedCapacity.convertDouble();
				dev.BeginRunTime = sinfo.BeginRunTime;
				dev.DeviceSubType = sinfo.DeviceSubType;
			}
			dev.SiteID = m_SiteID;
			dev.SiteName = m_SiteName;
			dev.RoomID = m_RoomID;
			dev.RoomName = m_RoomName;
			std::set<CData> attrSet;
			attrSet.insert("meterId");
			attrSet.insert("meterType");
			attrSet.insert("alarmLevel");
			attrSet.insert("threshold");
			attrSet.insert("meterName");
			std::list<std::map<CData,CData> > infoList;
			APPAPI::GetMeterInfo(aliasDevId, "alias", attrSet, infoList, 10000);
			for (auto mit=infoList.begin(); mit!=infoList.end(); mit++)
			{		
				std::map<CData,CData>& attr = *mit;
				CData meterId=attr["meterId"];
				if(meterId.length() > 0)
				{
					int alarmLevel = 0;
					float threshold = 0.0;

					if (1)
					{
						alarmLevel = attr["alarmLevel"].convertInt();
						threshold = attr["threshold"].convertDouble();
					}	
					TSignal signal;
					signal.Type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
					int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
					if (nType < 5)
					{
						signal.Type = nType;
					}
					else if(nType == 5)
					{
						signal.Type = CMM_SJY::ALARM;
					}
					signal.AlarmLevel = alarmLevel;
					signal.Threshold = threshold;
				
					int len=meterId.length();
					//取前面3位判断是否是移动量095或020
					CData id=meterId.substr(0,3);
					int iID=id.convertInt();
					bool bRet = Is_range(iID);
					if(len>3&& bRet)
					{						
						/*APPAPI::tMeterVal meterVal = { 0 };
						APPAPI::GetMeterVal(aliasDevId, meterId,"alias", meterVal);*/
						signal.ID =meterId.substr(0,len-3);
						signal.SignalNumber = meterId.substr(len-3,3).convertInt();
						signal.NMAlarmID = NMAlarmID(signal.ID);
						CData name  = GetDictionaryName(meterId);
						signal.SignalName = name;
						dev.singals.push_back(signal);
					}
				}
			}
			m_devCfg[aliasDevId] = dev;
		}
	}
	

	int CMMConfig::GetDevMetes(TDevConf& cfg)
	{
		ISFIT::SmartLock lock(m_devCfgMutex);
		if (m_devCfg.find(cfg.DeviceID) == m_devCfg.end())
		{
			return -1;
		}
		cfg = m_devCfg[cfg.DeviceID];
		return 0;
	}

	int CMMConfig::GetSemaphoreConf(CData devid, TSemaphore& cfg)
	{
		ISFIT::SmartLock lock(m_devCfgMutex);
		std::list<TSignal>::iterator pos = m_devCfg[devid].singals.begin();
		while (pos != m_devCfg[devid].singals.end())
		{
			if (pos->ID.compare(cfg.ID) == 0
				&& pos->SignalNumber == cfg.SignalNumber)
			{
				cfg.SetupVal = pos->SetupVal;
				cfg.Type = pos->Type;
				return 0;
			}
			pos++;
		}
		return -1;
	}

	void CMMConfig::SaveFile()
	{
		m_doc.Parse(CMM_DEVICE_CONFIG_FILE_HEADER);
		ISFIT::CXmlElement deviceList = m_doc.GetElement("Devices");
		int iNum = m_devCfg.size();
		deviceList.SetAttribute("Count", iNum);
		CMMProtocolEncode::AddDevicesInfo(deviceList);
		m_doc.SaveFile(m_DevCfgFileName.c_str());
	}


	int CMMConfig::GetDev(CData devId, TDevConf& cfg)
	{
		if (m_devCfg.find(devId) == m_devCfg.end())
		{
			return -1;
		}
		cfg = m_devCfg[devId];
		return 0;
	}


	//canyon rebuild
	int CMMConfig::SetDevCfg(std::map<CData, TDevConf>& devMap, std::list<CData>& scucessList, std::list<CData>& failList )
	{
		bool bOK=false;
		for (auto it=devMap.begin(); it!=devMap.end(); it++)
		{
			CData aliasDevId = it->first;
			TDevConf& devConf = it->second;

			std::map<CData, CData> devParamMap;
			APPAPI::GetDevParam(aliasDevId, "alias", devParamMap, 5000);
			CData deviceId = devParamMap["devId"];

			std::map<CData,CData> paramMap;
			paramMap["aliasDevName"] = devConf.DeviceName;
			paramMap["aliasDevId"] = aliasDevId;
			paramMap["devId"] = deviceId;
			APPAPI::SetDevParam(deviceId, "msj", paramMap,5000);

			LogInfo("deviceId:  " << deviceId);
			if (1)
			{
				/*auto iter = m_aliasId2Info.find(aliasDevId);
				if (iter != m_aliasId2Info.end())
				{
					TDeviceInfo& sinfo = iter->second;
					sinfo.Brand = devConf.Brand;
					sinfo.Model = devConf.Model;
					sinfo.Desc = devConf.DevDescribe;
					sinfo.Version = devConf.Version;
					sinfo.RatedCapacity = CData(devConf.RatedCapacity);
					sinfo.BeginRunTime = devConf.BeginRunTime;
				}
				m_SiteID = devConf.SiteID;
				m_SiteName = devConf.SiteName;
				m_RoomID = devConf.RoomID;
				m_RoomName = devConf.RoomName;*/
				std::list<std::map<CData,CData> > paramList;
				std::list<TSignal>& singals = devConf.singals;
				for (auto mit=singals.begin(); mit!=singals.end(); mit++)
				{	
					TSignal& signal = *mit;
					
					int signalNum=signal.SignalNumber;
					char tmp[32]={0};
					sprintf(tmp,"%03d",signalNum);
					CData strSignalNumber=CData(tmp);
					if (strSignalNumber == "000")
						strSignalNumber = "001";
						
					CData meterId = signal.ID+strSignalNumber;
					//LogInfo("~~~~~~~devid: " <<devid<<" id:"<< signal.ID<<" signalNum "<<strSignalNumber<<" meterID: "<<meterId);

					std::map <CData, CData> meterParamMap;
					meterParamMap["meterId"] = meterId;
					meterParamMap["threshold"] = CData(signal.Threshold);
					//LogInfo("threshold====:"<<CData(signal.Threshold));
					meterParamMap["alarmLevel"] = CData(signal.AlarmLevel);
					paramList.push_back(meterParamMap);
				}

				std::list<CData> errorMeterIdList;
				int ret1 = APPAPI::SetMeterParam(aliasDevId, "alias", paramList, errorMeterIdList, 5000);
				
				if (errorMeterIdList.size() == 0)
				{
					bOK = true;
					scucessList.push_back(aliasDevId);
					LogInfo("SetDevCfg ok devId:"<< aliasDevId <<" ret1:"<<ret1);
				}
				else
				{
					failList.push_back(aliasDevId);
					LogError("=== SetDevCfg() SetMeterParam failed devid:"<< aliasDevId <<" ret1:"<<ret1);
				}
			}
			else
			{
				failList.push_back(aliasDevId);
				LogError("=== SetDevCfg() SetDevParam failed devid:"<< aliasDevId);
			}
		}

       if(bOK)
	   {
			LogInfo("SetDevCfg===ok  now UpdateCfgFile");
			UpdateCfgFile();
			return 0;
	   }
	   
	   return -1;
		
	}

	int CMMConfig::GetDevConf( CData devid, TDevConf& cfg )
	{
		ISFIT::SmartLock lock(m_devCfgMutex);
		{
			auto it = m_devCfg.find(devid);
			if (it != m_devCfg.end())
			{
				cfg = it->second;
				return 0;
			}
		}
		LogError("get dev config failed id:"<<devid<<" go to get all devCfg");
		UpdateCfgFile();
		{
			auto it = m_devCfg.find(devid);
			if (it != m_devCfg.end())
			{
				cfg = it->second;
				return 0;
			}
		}
		LogError("~~~~~~get dev config still failed id:"<<devid);
		return -1;
	}

	void CMMConfig::GetSemaphoreConf(std::map<CData, std::list<TSemaphore>>& reqDevMap)
	{
		std::list <CData> devIdList;
		APPAPI::GetDevId("alias", devIdList);
		for (auto it = devIdList.begin(); it != devIdList.end(); it++)
		{
			std::list<TSemaphore> rspSemaphoreList;
			const CData& devId = *it;
			TDevConf cfg = { 0 };
			if (GetDevConf(devId, cfg) < 0)
			{
				LogError("get dev config failed id:" << devId);
				continue;
			}
			std::list<TSignal>::iterator pos = cfg.singals.begin();
			while (pos != cfg.singals.end())
			{
				CData NMAlarmID = pos->NMAlarmID;
				CData ID = pos->ID;
				int signalNum = pos->SignalNumber;
				char tmp[32] = { 0 };
				sprintf(tmp, "%03d", signalNum);
				CData strSignalNumber = CData(tmp);
				CData meterId = ID + strSignalNumber;
				std::map<CData, CData> attr;
				APPAPI::GetMeterInfo(devId, meterId, "alias", attr);
				if (attr["meterId"] != "")
				{
					TSemaphore rspSemaphore;
					rspSemaphore.ID = ID;
					rspSemaphore.SignalNumber = signalNum;
					rspSemaphore.AlarmLevel = pos->AlarmLevel;
					SetMeteValues(attr, rspSemaphore,pos->Type);
					rspSemaphoreList.push_back(rspSemaphore);
				}
				pos++;
			}
			reqDevMap[devId] = rspSemaphoreList;
		}
		LogInfo("GetSemaphoreConf size: " << reqDevMap.size());
	}

	int CMMConfig::SetSemaphoreConf( CData devid, TSemaphore& cfg )
	{
	   TDevConf devCfg = {0};
		if(GetDevConf(devid, devCfg) < 0)
		{
			return -1;
		}
		TSemaphore& semaphore = cfg;
		int signalNum= semaphore.SignalNumber;
		char tmp[32]={0};
		sprintf(tmp,"%03d",signalNum);
		CData strSignalNumber=CData(tmp);		
		CData meterId = semaphore.ID + strSignalNumber;
		std::map<CData,CData> paramMap;
		CData setupVal(semaphore.SetupVal);
		paramMap["val"] = setupVal;
		//LogInfo("~~~~~~~devid: " <<devid.c_str() <<" meterId:"<<meterId.c_str() <<" setupVal "<<setupVal.c_str());
		int ret = APPAPI::SetMeterVal(devid, meterId, "alias", setupVal);
		if (ret<0) return -2;
		APPAPI::SetMeterParam(devid, meterId, "alias", paramMap);
		return 0;
	}

	int CMMConfig::SetThresholdConf( CData devid, TThreshold& cfg )
	{
		//CData id = cfg.ID.substr(0,3);
		//if (id != "020") return -1;
		TDevConf devCfg = { 0 };
		if (GetDevConf(devid, devCfg) < 0)
		{
			return -1;
		}
		int signalNum = cfg.SignalNumber;
		char tmp[32] = { 0 };
		sprintf(tmp, "%03d", signalNum);
		CData strSignalNumber = CData(tmp);
		CData meterId = cfg.ID + strSignalNumber;
		//LogInfo("~~~~~~~~~devid: " << devid << "meterID: " << meterId);
		std::map<CData, CData> paramMap;
		paramMap["threshold"] = CData(cfg.Threshold);
		paramMap["alarmLevel"] = CData(cfg.AlarmLevel);
		APPAPI::SetMeterParam(devid, meterId, "alias", paramMap);
		return 0;
	}

	int CMMConfig::SetStorageRuleConf( CData devid, TSignal& cfg )
	{
		TDevConf devCfg = {0};
		if(GetDevConf(devid, devCfg) < 0)
		{
			return -1;
		}
		TSignal& signal = cfg;
		int signalNum= signal.SignalNumber;
		char tmp[32]={0};
		sprintf(tmp,"%03d",signalNum);
		CData strSignalNumber=CData(tmp);
		CData meterId = signal.ID + strSignalNumber;
		//LogInfo("~~~~~~~~~~~~~~devid: " <<devid<<" id:"<< signal.ID<<"signalNum "<<strSignalNumber<<"meterID: "<<meterId);			
		std::map<CData,CData> paramMap;
		paramMap["absoluteVal"] = CData(signal.AbsoluteVal);
		paramMap["relativeVal"] = CData(signal.RelativeVal);
		paramMap["storePeriod"] = CData(signal.savePeriod);
		APPAPI::SetMeterParam(devid, meterId, "alias", paramMap);
		return 0;
	}

	int CMMConfig::SetMeteValues(std::map<CData, CData>& param, TSemaphore& semaphore,int nType)
	{
		semaphore.MeasuredVal = param["val"].convertDouble();
		semaphore.Status = CMM_SJY::STATE_NOALARM;
		//LogInfo("~~~~~~~~~~~~~~pMeter->GetTime："<<pMeter->GetTime());
		semaphore.Time = param["time"];
		semaphore.Type = nType;
		semaphore.SetupVal = param["setupVal"].convertDouble();
		return 0;
	}

	int CMMConfig::SetMeteStorageRule(std::map<CData, CData>& param, TSignal& Signal, int nType)
	{
		Signal.AbsoluteVal = param["absoluteVal"].convertDouble();
		Signal.RelativeVal = param["relativeVal"].convertDouble();
		Signal.savePeriod = param["storePeriod"].convertInt();
		Signal.Type = nType;
		return 0;
	}


	int CMMConfig::SetMeteThreshold(std::map<CData, CData>& param, TThreshold& theshold, int nType)
	{
		theshold.Threshold = param["threshold"].convertDouble();
		theshold.Status = STATE_NOALARM;
		theshold.AlarmLevel = param["alarmLevel"].convertInt();
		theshold.Type = nType;
		return 0;
	}

	CData CMMConfig::GetFsuId()
	{
		return m_fsuId;
	}

	void CMMConfig::SetFsuId( CData fsuId )
	{
		m_fsuId = fsuId;
	}

	CData CMMConfig::GetUserName()
	{
		return m_userName;
	}

	void CMMConfig::SetUserName( CData userName,bool saveDb )
	{
		m_userName = userName;
		if (saveDb)
		{
			SetParam(CMM_SJY::param::UserName, userName);
		}
	}


	CData CMMConfig::GetPassword()
	{
		return m_password;
	}

	void CMMConfig::SetPassword( CData password,bool saveDb )
	{
		m_password = password;
		if (saveDb)
			SetParam(CMM_SJY::param::Password, password);
	}

	void CMMConfig::SetFsuPort( CData port )
	{
		m_fsuPort = port;
	}

	void CMMConfig::SetUdpPort(CData port)
	{
		m_udpPort = port;
	}

	CData CMMConfig::GetFsuPort()
	{
		return m_fsuPort;
	}
	
	int CMMConfig::SetFtpUsr(CData usr, bool saveDb)
	{
		if (m_ftpUsr == usr)
		{
			LogInfo("CMMConfig::SetFtpUsr usr:"<<usr<<" no changes, no need save");
			return 0;
		}
		
		m_ftpUsr = usr;
		if (saveDb)
		{
			SetParam(CMM_SJY::param::FtpUsr, usr);
		}
		else
		{
			CMMAccess::instance()->AddLinuxSysUser(m_ftpUsr, "", "/");
			//CMMAccess::instance().ModifyLinuxSysPasswd(m_ftpUsr, m_ftpPasswd);
		}

		return 0;	// DAHAI
	}

	int CMMConfig::SetFtpPasswd( CData passwd , bool saveDb)
	{
		if (m_ftpPasswd == passwd)
		{
			LogInfo("CMMConfig::SetFtpPasswd passwd:"<<passwd<<" no changes, no need save");
			return 0;
		}
		
		m_ftpPasswd = passwd;
		if (saveDb)
		{
			return SetParam(CMM_SJY::param::FtpPasswd, passwd);
		}
		else
		{
			//CMMAccess::instance().AddLinuxSysUser(m_ftpUsr, "", "/");
			int ret=CMMAccess::instance()->ModifyLinuxSysPasswd(m_ftpUsr, m_ftpPasswd);
			return ret;
		}
	}

	CData CMMConfig::GetFsuIp()
	{
		CData ip;
		CData content = ISFIT::Shell("ifconfig eth0");
		CData pattern = "inet addr:";
		int startPos = content.find(pattern);
		if (CDATA_NPOS != startPos)
		{
			int endPos =  content.find(" ", startPos+pattern.size());
			if (CDATA_NPOS != endPos)
			{
				ip = content.substring(startPos+pattern.size(), endPos);
			}
		}
		if (ip.size() > 0) return ip;
		return m_fsuIp;
	}

	void CMMConfig::SetFsuIp(CData ip )
	{
		m_fsuIp = ip;
	}

	void CMMConfig::addAcceptIP(CData familyType, std::list<CData>& IPList)
	{
		m_familyIPList[familyType] = IPList;
	}

	bool CMMConfig::isAcceptIp(CData familyType, CData ip)
	{
		if (m_familyIPList.size() == 0)  //为空则不拦截ip
		{
			return true;
		}
		if (ip.compare("127.0.0.1") == 0)  //不拦截外网
		{
			return true;
		}
		std::list <CData> ipList = m_familyIPList[familyType];
		for (auto it = ipList.begin(); it != ipList.end(); ++it)
		{
			CData strIp = *it;
			if (strIp.compare(ip) == 0)
			{
				return true;
			}
		}
		return false;
	}

	CData CMMConfig::CreateMeasurefile(CData& timestamp)
	{
		CData measureFile = "/Measurement/";
		// 创建一个LocalDateTime对象，表示当前时间
		Poco::Timestamp now;
		Poco::LocalDateTime local(now);
		// 提取年、月、日
		int year = local.year();
		int month = local.month();
		int day = local.day();
		int hour = local.hour();
		int minute = local.minute();
		std::stringstream ss;
		ss << std::setfill('0')
			<< std::setw(4) << year
			<< std::setw(2) << month
			<< std::setw(2) << day
			<< std::setw(2) << hour
			<< std::setw(2) << minute;
		// 获取最终的字符串
		std::string formattedTimestamp = ss.str();
		timestamp = formattedTimestamp;
		measureFile += "PM_";
		measureFile += m_fsuId + "_";
		measureFile += formattedTimestamp + ".csv";
		return measureFile;
	}

	bool CMMConfig::WriteMeasurefile()
	{
		
		if (m_fsuId.empty() || m_fsuId.length() < 1)
		{
			//LogError("3333333333333");
			return false;
		}
			
		LogInfo("WriteMeasurefile begin:" << m_fsuId.c_str());
		UpdateCfgFile();
		std::map<CData, std::list<TSemaphore>> reqDevMap;
		GetSemaphoreConf(reqDevMap);
		if (reqDevMap.size() == 0)
			return false;
		CData timestamp;
		CData filePath = CreateMeasurefile(timestamp);

		// 打开文件以进行写入  
		std::ofstream outputFile(filePath.c_str());
		if (!outputFile) 
		{
			LogError("Failed to open output file!");
			return false; // 或者处理错误，例如退出程序  
		}

		CData head = "序号,性能数据采集时间,DeviceID,监控点ID,SignalNumber,监控点描述,监控点数据类型,监控点数据测量值\n";//文件数据格式
		outputFile << head.c_str();
		auto it = reqDevMap.begin();
		int SerialNo= 1;
		for (; it != reqDevMap.end(); ++it)
		{
			CData deviceId = it->first;
			std::list<TSemaphore> semInfos = it->second;
			auto iter = semInfos.begin();
			for (; iter != semInfos.end(); ++iter)
			{
				TSemaphore semInfo = *iter;
				if (semInfo.Type < 3) 
					continue;
				if(semInfo.AlarmLevel > 0) //只需要遥信AI = 3, 遥测DI =4 遥测包含告警 因此过滤告警级别大于0的（为0才是DI 否则是告警）
					continue;
				CData type = "DI";
				if (semInfo.Type == 3)
					type = "AI";

				std::string time = semInfo.Time.c_str(); //2024-02-28 17:32:12
				for (char c : {' ', '-', ':'}) 
				{
					// 使用 erase-remove idiom 移除特定字符
					time.erase(std::remove(time.begin(), time.end(), c), time.end());
				}
				std::ostringstream os;
				os << semInfo.ID.c_str() << std::setw(3) << std::setfill('0') << 1;
				CData meterId = os.str();
				CData meterName = GetDictionaryName(meterId);
				std::ostringstream oss;
				oss << SerialNo++ << ","
					<< time << ","
					<< deviceId.c_str() << ","
					<< semInfo.ID.c_str() << ","
					<< std::setw(3) << std::setfill('0') << semInfo.SignalNumber << ","
					<< meterName.c_str() << ","
					<< type.c_str() << ","
					<< semInfo.MeasuredVal << "\n";

				std::string strInfo = oss.str();
				outputFile << strInfo;
			}	
		}
		// 关闭文件流  
		outputFile.close();
		// 检查文件是否成功关闭  
		if (!outputFile.good()) 
		{
			LogError("Failed to write to output file!");
			return false;
		}
		return true;
	}
}




