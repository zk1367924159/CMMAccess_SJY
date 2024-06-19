#include "CMMProtocolEncode.h"
#include "CLog.h"
#include "CMMCommonStruct.h"
#include "CMMConfig.h"
#include "NetModule.h"
#include "NetComm/ModuleVersions.h"
#include "CMMMeteTranslate.h"
#include "CMMConfig.h"
#include "Poco/MD5Engine.h"
#include "Poco/DigestStream.h"
#include "SysCommon.h"
#include "CMMAccess.h"
#include "CTextEncryption.h"
#include "../../ExtAppIpc/ExtAppIpcApi.h"


#define SPECIAL_METERID "095402"
namespace CMM_SJY{
#define  SET_XML_ATTRIBUTE(element, attr, value) element.SetAttribute(attr, value);
	CData CMMProtocolEncode::BuildLogMsg()
	{			
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_REQUEST_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Request);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::LOGIN);
	
			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);
	
			info.AddSubElement(CMM_SJY::UserName).SetElementText(CMMConfig::instance()->m_userName.c_str());
			CData hashPasswd;
			CTextEncryption::hashMessage(CMMConfig::instance()->GetPassword(), hashPasswd, 0);
			info.AddSubElement(CMM_SJY::PassWord).SetElementText(hashPasswd.c_str());
			info.AddSubElement(CMM_SJY::AlgType).SetElementText(0);
			info.AddSubElement(CMM_SJY::FSUID).SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			CData mac = CMMAccess::instance()->GetLocalMac();
			CData localIp = CMMConfig::instance()->GetFsuIp();		// DAHAI
			info.AddSubElement(CMM_SJY::FSUIP).SetElementText("49.234.66.123");
			info.AddSubElement(CMM_SJY::FSUMAC).SetElementText(mac.c_str());

			std::map<CData,CData> verMap;
			APPAPI::ReadVersion(verMap);
			CData ver = verMap["appVer"];
			//info.AddSubElement(CMM_SJY::FSUVER).SetElementText(ver.c_str()); //V2.03.39-beta15
			info.AddSubElement(CMM_SJY::FSUVER).SetElementText("4.5.0");
			//info.AddSubElement("FSUPORT").SetElementText(CMMConfig::instance()->GetFsuPort().c_str());
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();		
	}

	CData CMMProtocolEncode::ReportDevConf()
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_REQUEST_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Request);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::SEND_DEV_CONF_DATA);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement(CMM_SJY::FSUID).SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			ISFIT::CXmlElement values = info.AddSubElement(CMM_SJY::Values);
			//ISFIT::CXmlElement DeviceList = values.AddSubElement(CMM_SJY::DeviceList);
			AddDevicesInfo(values);
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}


	void CMMProtocolEncode::AddDevicesInfo(ISFIT::CXmlElement& DeviceList, std::list<CData> &idList )
	{
		std::map<CData, TDevConf> devices = CMMConfig::instance()->GetDevices();
		std::map<CData, TDevConf>::iterator pos = devices.begin();
		while(pos != devices.end())
		{
			if(idList.size() > 0)
			{
				std::list<CData>::iterator devPos = idList.begin();
				bool bFound = false;
				while(devPos != idList.end())
				{
					if(devPos->compare(pos->second.DeviceID) == 0)
					{
						bFound = true;
						break;
					}
					devPos++;
				}
				if(bFound == false)
				{
					pos++;
					continue;
				}
			}
			ISFIT::CXmlElement device = DeviceList.AddSubElement(CMM_SJY::Device);	

			TDevConf &dev = pos->second;
			SET_XML_ATTRIBUTE(device, "DeviceID", dev.DeviceID.c_str());
			SET_XML_ATTRIBUTE(device, "DeviceName", dev.DeviceName.c_str());
			SET_XML_ATTRIBUTE(device, "SiteID",dev.SiteID.c_str());
			SET_XML_ATTRIBUTE(device, "RoomID", dev.RoomID.c_str());
			SET_XML_ATTRIBUTE(device, "SiteName", dev.SiteName.c_str());
			SET_XML_ATTRIBUTE(device, "RoomName", dev.RoomName.c_str());
			SET_XML_ATTRIBUTE(device, "DeviceType", dev.DeviceType);
			SET_XML_ATTRIBUTE(device, "DeviceSubType", dev.DeviceSubType);
			SET_XML_ATTRIBUTE(device, "Model", dev.Model.c_str());
			SET_XML_ATTRIBUTE(device, "Brand", dev.Brand.c_str());
			SET_XML_ATTRIBUTE(device, "RatedCapacity", dev.RatedCapacity);
			SET_XML_ATTRIBUTE(device, "Version", dev.Version.c_str());
			SET_XML_ATTRIBUTE(device, "BeginRunTime", dev.BeginRunTime.c_str());
			SET_XML_ATTRIBUTE(device, "DevDescribe", dev.DevDescribe.c_str());			
			SET_XML_ATTRIBUTE(device, "ConfRemark", dev.ConfRemark.c_str());
			
			ISFIT::CXmlElement Signals = device.AddSubElement("Signals");
			int iNum=pos->second.singals.size();
			SET_XML_ATTRIBUTE(Signals, "Count", iNum);
			std::list<TSignal>::iterator metePos =  pos->second.singals.begin();
			while(metePos != pos->second.singals.end())
			{	
				ISFIT::CXmlElement TSignal = Signals.AddSubElement("Signal");
				SET_XML_ATTRIBUTE(TSignal, "Type", metePos->Type);		
				SET_XML_ATTRIBUTE(TSignal, "ID", metePos->ID.c_str());
				SET_XML_ATTRIBUTE(TSignal, "SignalName", metePos->SignalName.c_str());
				//SET_XML_ATTRIBUTE(TSignal, "Describe", metePos->Describe.c_str());
				SET_XML_ATTRIBUTE(TSignal, "AlarmLevel", metePos->AlarmLevel);
				SET_XML_ATTRIBUTE(TSignal, "Threshold", metePos->Threshold);
				//SET_XML_ATTRIBUTE(TSignal, "AbsoluteVal", metePos->AbsoluteVal);
				//SET_XML_ATTRIBUTE(TSignal, "SavePeriod", metePos->savePeriod);
				//SET_XML_ATTRIBUTE(TSignal, "RelativeVal", metePos->RelativeVal);
				SET_XML_ATTRIBUTE(TSignal, "NMAlarmID",metePos->NMAlarmID.c_str());
				char strSignalNum[32]={0};
				sprintf(strSignalNum,"%03d",metePos->SignalNumber);
				SET_XML_ATTRIBUTE(TSignal, "SignalNumber", strSignalNum);		
				//SET_XML_ATTRIBUTE(TSignal, "SignalNumber", metePos->SignalNumber);		
			//	SET_XML_ATTRIBUTE(TSignal, "SetupVal", metePos->SetupVal);
				metePos++;
			}
			pos++;
		}
		
	}

	void CMMProtocolEncode::AddDevicesInfo( ISFIT::CXmlElement& devList )
	{
		std::list<CData> idList;
		AddDevicesInfo(devList, idList);
	}

	CData CMMProtocolEncode::GetDevConf( std::list<CData>& devList )
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::GET_DEV_CONF_ACK);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("Result").SetElementText(CMM_SJY::SUCCESS);
			info.AddSubElement("FailureCause").SetElementText("NULL");
			ISFIT::CXmlElement values = info.AddSubElement(CMM_SJY::Values);
			//ISFIT::CXmlElement DeviceList = values.AddSubElement(CMM_SJY::DeviceList);
			AddDevicesInfo(values, devList);
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::GetFsuInfo()
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::GET_FSUINFO_ACK);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("Result").SetElementText(CMM_SJY::SUCCESS);
			info.AddSubElement("FailureCause").SetElementText("NULL");
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			ISFIT::CXmlElement TFSUStatus =  info.AddSubElement("TFSUStatus");
			ISFIT_OS::T_SysInfo sysinfo;
			ISFIT_OS::getsysInfo(sysinfo);

			//TFSUStatus.AddSubElement("CPUUsage").SetElementText((float)12.9);
			//TFSUStatus.AddSubElement("MEMUsage").SetElementText((float)18.8);
			//TFSUStatus.AddSubElement("HardDiskUsage").SetElementText((float)66.9);
			
			TFSUStatus.AddSubElement("CPUUsage").SetElementText(sysinfo.cpuUsage);
			TFSUStatus.AddSubElement("MEMUsage").SetElementText(sysinfo.memUsage);
			TFSUStatus.AddSubElement("HardDiskUsage").SetElementText((float)0.0);
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::GenGeneralSetRsp(int result,CData FailureCause,CData type, std::list<CData> scucessList, std::list<CData> failList )
	{
	ISFIT::CXmlDoc doc;
	doc.Parse(CMM_RESPONSE_XML_HEAD);
	try
	{
	ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
	ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
	pkType.GetSubElement(CMM_SJY::Name).SetElementText(type.c_str());

	ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
	//info.AddSubElement("Result").SetElementText(CMM_SJY::SUCCESS);
	//info.AddSubElement("FailureCause").SetElementText("NULL");
	info.AddSubElement("Result").SetElementText(result);
	info.AddSubElement("FailureCause").SetElementText(FailureCause.c_str());
	info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
	ISFIT::CXmlElement Success =  info.AddSubElement("SuccessList");
	std::list<CData>::iterator pos = scucessList.begin();
	while(pos != scucessList.end())
	{
	ISFIT::CXmlElement Device = Success.AddSubElement("Device");
	SET_XML_ATTRIBUTE(Device, "ID", (*pos).c_str());
	pos++;
	}
	ISFIT::CXmlElement Failed = info.AddSubElement("FailList");
	pos = failList.begin();
	while(pos != failList.end())
	{
	ISFIT::CXmlElement Device = Failed.AddSubElement("Device");
	SET_XML_ATTRIBUTE(Device, "ID", (*pos).c_str());
	pos++;
	}
	}
	catch (Poco::Exception& ex)
	{
	LogError("build login msg failed :"<<ex.message().c_str());
	return "";
	}
	return doc.ToString();	
	}

	CData CMMProtocolEncode::BuildGetDataRsp(int result,  std::map<CData, std::list<TSemaphore> > &devMap )
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::GET_DATA_ACK);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("Result").SetElementText(result);
			if(result == CMM_SJY::SUCCESS || result == CMM_SJY::NODATA)
			{
				info.AddSubElement("FailureCause").SetElementText("NULL");
			}
			else
			{
				info.AddSubElement("FailureCause").SetElementText("获取监控点数据失败");
			}
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			ISFIT::CXmlElement Values =  info.AddSubElement("Values");
			ISFIT::CXmlElement DeviceList = Values.AddSubElement("DeviceList");
			if(result == CMM_SJY::SUCCESS)
			{
				std::map<CData, std::list<TSemaphore> >::iterator pos = devMap.begin();
				while(pos != devMap.end())
				{
					ISFIT::CXmlElement Device  = DeviceList.AddSubElement("Device");
					SET_XML_ATTRIBUTE(Device,"ID", pos->first.c_str());		
					std::list<TSemaphore>::iterator idPos = pos->second.begin();
					while(idPos != pos->second.end())
					{
						ISFIT::CXmlElement TSemaphore  = Device.AddSubElement("TSemaphore");
						SET_XML_ATTRIBUTE(TSemaphore, "Type", idPos->Type);
						SET_XML_ATTRIBUTE(TSemaphore,"ID", idPos->ID.c_str());
						SET_XML_ATTRIBUTE(TSemaphore,"MeasuredVal", idPos->MeasuredVal);
					//	SET_XML_ATTRIBUTE(TSemaphore,"SetupVal", idPos->SetupVal);
					   SET_XML_ATTRIBUTE(TSemaphore,"SetupVal", "NULL");
						SET_XML_ATTRIBUTE(TSemaphore,"Status", idPos->Status);
						SET_XML_ATTRIBUTE(TSemaphore,"Time", idPos->Time.c_str());
						
						char strSignalNum[32]={0};
						sprintf(strSignalNum,"%03d",idPos->SignalNumber);
						SET_XML_ATTRIBUTE(TSemaphore,"SignalNumber", strSignalNum);
						//SET_XML_ATTRIBUTE(TSemaphore,"SignalNumber", idPos->SignalNumber);
						idPos++;
					}
					pos++;
				}
			}			
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::BuildGetStorageRuleRsp(int result,  std::map<CData, std::list<TSignal> > &devMap )
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		time_t nowTime = ISFIT::getLocalTime();;
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::GET_STORAGERULE_ACK);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("Result").SetElementText(result);
			if(result == CMM_SJY::SUCCESS)
			{
				info.AddSubElement("FailureCause").SetElementText("NULL");
			}
			else
			{
				info.AddSubElement("FailureCause").SetElementText("获取监控点存储规则失败");
			}
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			ISFIT::CXmlElement Values =  info.AddSubElement("Values");
			ISFIT::CXmlElement DeviceList = Values.AddSubElement("DeviceList");
			if(result == CMM_SJY::SUCCESS)
			{
				std::map<CData, std::list<TSignal> >::iterator pos = devMap.begin();
				while(pos != devMap.end())
				{
					ISFIT::CXmlElement Device  = DeviceList.AddSubElement("Device");
					SET_XML_ATTRIBUTE(Device,"ID", pos->first.c_str());		
					std::list<TSignal>::iterator idPos = pos->second.begin();
					while(idPos != pos->second.end())
					{
						ISFIT::CXmlElement StorageRule  = Device.AddSubElement("TStorageRule");								
						SET_XML_ATTRIBUTE(StorageRule,"Type", idPos->Type);
						SET_XML_ATTRIBUTE(StorageRule,"ID", idPos->ID.c_str());
						SET_XML_ATTRIBUTE(StorageRule,"AbsoluteVal", idPos->AbsoluteVal);
						SET_XML_ATTRIBUTE(StorageRule,"RelativeVal", idPos->RelativeVal);
						SET_XML_ATTRIBUTE(StorageRule,"StorageInterval", idPos->savePeriod);
						SET_XML_ATTRIBUTE(StorageRule,"StorageRefTime", ISFIT::timeToString(nowTime).data());
						char strSignalNum[32]={0};
						sprintf(strSignalNum,"%03d",idPos->SignalNumber);
						SET_XML_ATTRIBUTE(StorageRule,"SignalNumber", strSignalNum);
						//SET_XML_ATTRIBUTE(StorageRule,"SignalNumber", idPos->SignalNumber);
						idPos++;
					}
					pos++;
				}
			}			
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::BuildAalrmReportInfo(std::list<TAlarm>&alarmList)
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_REQUEST_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Request);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::SEND_ALARM);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			ISFIT::CXmlElement Values = info.AddSubElement("Values");
			ISFIT::CXmlElement TAlarmList = Values.AddSubElement("TAlarmList");
			std::list<TAlarm>::iterator pos = alarmList.begin();
			while(pos != alarmList.end())
			{
				if(pos->retryTimes >= 3)
				{
					alarmList.erase(pos++);
					continue;
				}
				else
				{
					if(pos->retryTimes > 0)
					{
						Poco::Timestamp now;
						int diff = now.epochTime() - pos->lastReportTime.epochTime();
						if(diff < 30)
						{
							pos++;
							continue;
						}
					}
				}
				ISFIT::CXmlElement alarm = TAlarmList.AddSubElement("TAlarm");
				alarm.AddSubElement("SerialNo").SetElementText(pos->SerialNo.c_str());
				alarm.AddSubElement("ID").SetElementText(pos->ID.c_str());
				alarm.AddSubElement("DeviceID").SetElementText(pos->DeviceID.c_str());
				alarm.AddSubElement("NMAlarmID").SetElementText(pos->NMAlarmID.c_str());
				alarm.AddSubElement("AlarmTime").SetElementText(pos->AlarmTime.c_str());
				alarm.AddSubElement("AlarmLevel").SetElementText(pos->AlarmLevel);
				alarm.AddSubElement("EventValue").SetElementText(pos->EventValue);
				alarm.AddSubElement("AlarmFlag").SetElementText(pos->AlarmFlag.c_str());

				alarm.AddSubElement("AlarmDesc").SetElementText(pos->AlarmDesc.c_str());
				alarm.AddSubElement("AlarmRemark").SetElementText(pos->AlarmRemark1.c_str());
				//alarm.AddSubElement("AlarmRemark2").SetElementText(pos->AlarmRemark2.c_str());
				char strSignalNum[32]={0};
				sprintf(strSignalNum,"%03d",pos->SignalNumber);
				alarm.AddSubElement("SignalNumber").SetElementText(strSignalNum);
				//alarm.AddSubElement("SignalNumber").SetElementText(pos->SignalNumber);
				pos->retryTimes++;
				pos->lastReportTime.update();
				pos++;
			}
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::BuildGetThresholdRsp(int result,  std::map<CData, std::list<TThreshold> >&devMap )
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::GET_THRESHOLD_ACK);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("Result").SetElementText(result);
			if(result == CMM_SJY::SUCCESS)
			{
				info.AddSubElement("FailureCause").SetElementText("NULL");
			}
			else
			{
				info.AddSubElement("FailureCause").SetElementText("获取监控点门限数据失败");
			}			
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			ISFIT::CXmlElement Values =  info.AddSubElement("Values");
			ISFIT::CXmlElement DeviceList = Values.AddSubElement("DeviceList");
			if(result == CMM_SJY::SUCCESS)
			{
				std::map<CData, std::list<TThreshold> >::iterator pos = devMap.begin();
				while(pos != devMap.end())
				{
					ISFIT::CXmlElement Device  = DeviceList.AddSubElement("Device");
					SET_XML_ATTRIBUTE(Device, "ID", pos->first.c_str());		
					std::list<TThreshold>::iterator idPos = pos->second.begin();

					while(idPos != pos->second.end())
					{
						ISFIT::CXmlElement TSemaphore  = Device.AddSubElement("TThreshold");						
						SET_XML_ATTRIBUTE(TSemaphore, "Type", idPos->Type);		
						SET_XML_ATTRIBUTE(TSemaphore, "ID", idPos->ID.c_str());
						SET_XML_ATTRIBUTE(TSemaphore, "Threshold", idPos->Threshold);
					//	SET_XML_ATTRIBUTE(TSemaphore, "AbsoluteVal", idPos->AbsoluteVal);
						//SET_XML_ATTRIBUTE(TSemaphore, "RelativeVal", idPos->RelativeVal);
					//	SET_XML_ATTRIBUTE(TSemaphore, "Status", idPos->Status);
						char strSignalNum[32]={0};
						sprintf(strSignalNum,"%03d",idPos->SignalNumber);
						SET_XML_ATTRIBUTE(TSemaphore, "SignalNumber", strSignalNum);
						//SET_XML_ATTRIBUTE(TSemaphore, "SignalNumber", idPos->SignalNumber);
						SET_XML_ATTRIBUTE(TSemaphore, "AlarmLevel", idPos->AlarmLevel);
						SET_XML_ATTRIBUTE(TSemaphore, "NMAlarmID", idPos->NMAlarmID.c_str());
						idPos++;
					}
					pos++;
				}
			}			
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::BuildDataReport(std::map<CData, std::list<TSemaphore>>& mapSem)
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_REQUEST_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Request);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::SEND_DATA);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			ISFIT::CXmlElement Values = info.AddSubElement("Values");
			ISFIT::CXmlElement DeviceList = Values.AddSubElement("DeviceList");

			std::map<CData, std::list<TSemaphore>>::iterator iter = mapSem.begin();
			while (iter != mapSem.end())
			{
				CData deviceId = iter->first;
				std::list<TSemaphore> semList = iter->second;
				ISFIT::CXmlElement Device = DeviceList.AddSubElement("Device");
				SET_XML_ATTRIBUTE(Device, "ID", deviceId.c_str());

				std::list<TSemaphore>::iterator iter2 = semList.begin();
				while (iter2 != semList.end()) 
				{
					TSemaphore semInfo = *iter2;
					ISFIT::CXmlElement TSemaphore = Device.AddSubElement("TSemaphore ");
					SET_XML_ATTRIBUTE(TSemaphore, "ID", semInfo.ID.c_str());			
					SET_XML_ATTRIBUTE(TSemaphore, "Type", semInfo.Type);
					SET_XML_ATTRIBUTE(TSemaphore, "MeasuredVal", semInfo.MeasuredVal);
					//SET_XML_ATTRIBUTE(TSemaphore, "SetupVal", semaphore.SetupVal);
					SET_XML_ATTRIBUTE(TSemaphore, "SetupVal", "NULL");
					SET_XML_ATTRIBUTE(TSemaphore, "Status", semInfo.Status);
					char strSignalNum[32] = { 0 };
					sprintf(strSignalNum, "%03d", semInfo.SignalNumber);
					SET_XML_ATTRIBUTE(TSemaphore, "SignalNumber", strSignalNum);
					//SET_XML_ATTRIBUTE(TSemaphore, "SignalNumber", semaphore.SignalNumber);
					SET_XML_ATTRIBUTE(TSemaphore, "Time", semInfo.Time.c_str());
					iter2++;
				}
				iter++;
			}
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::BuildGetFtpInfoRsp()
	{

		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::GET_FTP_ACK);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("Result").SetElementText(CMM_SJY::SUCCESS);
			info.AddSubElement("FailureCause").SetElementText("NULL");
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			/*CData user, password;
			FSUUTIL::GetFtpUser(user, password);
			info.AddSubElement("UserName").SetElementText(user.c_str());
			info.AddSubElement("PassWord").SetElementText(password.c_str());*/
			
			info.AddSubElement("UserName").SetElementText(CMMConfig::instance()->m_ftpUsr.c_str());
			info.AddSubElement("PassWord").SetElementText(CMMConfig::instance()->m_ftpPasswd.c_str());						
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::BuildGeneralRsp( int result, CData reason, CData type )
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(type.c_str());

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("Result").SetElementText(result);
			info.AddSubElement("FailureCause").SetElementText(reason.c_str());
			//info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::BuildGetLoginInfoRsp()
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(CMM_SJY::method::GET_LOGININFO_ACK);

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);			
			info.AddSubElement("Result").SetElementText(CMM_SJY::SUCCESS);
			info.AddSubElement("FailureCause").SetElementText("NULL");
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			info.AddSubElement("UserName").SetElementText(CMMConfig::instance()->GetUserName().c_str());
			info.AddSubElement("PassWord").SetElementText(CMMConfig::instance()->GetPassword().c_str());
			CData mac = CMMAccess::instance()->GetLocalMac();
			CData localIp = CMMAccess::instance()->GetIfcIp("eth0");
			info.AddSubElement("FSUIP").SetElementText(localIp.c_str());
			info.AddSubElement("FSUMAC").SetElementText(mac.c_str());
			
			std::map<CData,CData> verMap;
			APPAPI::ReadVersion(verMap);
			CData ver = verMap["appVer"];
			info.AddSubElement("FSUVER").SetElementText("4.5.0");
			
			info.AddSubElement("SiteID").SetElementText(CMMConfig::instance()->GetSiteID().c_str());
			info.AddSubElement("RoomID").SetElementText(CMMConfig::instance()->GetRoomID().c_str());
			info.AddSubElement("SiteName").SetElementText(CMMConfig::instance()->GetSiteName().c_str());
			info.AddSubElement("RoomName").SetElementText(CMMConfig::instance()->GetRoomName().c_str());

		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :"<<ex.message().c_str());
			return "";
		}
		return doc.ToString();	
	}

	CData CMMProtocolEncode::BuildGetTimeRsp(int result, CData reason, CData type)
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(type.c_str());

			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);
			info.AddSubElement("Result").SetElementText(result);
			info.AddSubElement("FailureCause").SetElementText(reason.c_str());
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());

			ISFIT::CXmlElement time = info.AddSubElement(CMM_SJY::Time);

			Poco::Timestamp now;
			Poco::LocalDateTime localDateTime(now);
			// 提取年、月、日
			int year = localDateTime.year();
			int month = localDateTime.month();
			int day = localDateTime.day();
			int hour = localDateTime.hour();
			int minute = localDateTime.minute();
			int second = localDateTime.second();

			time.AddSubElement("Year").SetElementText(year);
			time.AddSubElement("Month").SetElementText(month);
			time.AddSubElement("Day").SetElementText(day);
			time.AddSubElement("Hour").SetElementText(hour);
			time.AddSubElement("Minute").SetElementText(minute);
			time.AddSubElement("Second").SetElementText(second);
		}
		catch (Poco::Exception& ex)
		{
			LogError("build get time failed :" << ex.message().c_str());
			return "";
		}
		return doc.ToString();
	}

	CData CMMProtocolEncode::BuildAuthErrorMsg(CData type)
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			CData strAck = type + "_ACK";
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(strAck.c_str());
			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			info.AddSubElement("Result").SetElementText(3);
			info.AddSubElement("FailureCause").SetElementText("failed verify to Authorization");
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :" << ex.message().c_str());
			return "";
		}
		return doc.ToString();
	}

	CData CMMProtocolEncode::BuildLoginErrorMsg(CData type)
	{
		ISFIT::CXmlDoc doc;
		doc.Parse(CMM_RESPONSE_XML_HEAD);
		try
		{
			ISFIT::CXmlElement root = doc.GetElement(CMM_SJY::Response);
			ISFIT::CXmlElement pkType = root.GetSubElement(CMM_SJY::PK_Type);
			CData strAck = type + "_ACK";
			pkType.GetSubElement(CMM_SJY::Name).SetElementText(strAck.c_str());
			ISFIT::CXmlElement info = root.AddSubElement(CMM_SJY::Info);
			info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
			info.AddSubElement("Result").SetElementText(6);
			info.AddSubElement("FailureCause").SetElementText("Device not registered or registration expired");
		}
		catch (Poco::Exception& ex)
		{
			LogError("build login msg failed :" << ex.message().c_str());
			return "";
		}
		return doc.ToString();
	}
}


