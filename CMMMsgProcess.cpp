#include "CMMMsgProcess.h"
#include "CLog.h"
#include "CMMProtocolEncode.h"
#include "CMMAccess.h"
#include "CMMProtocolDecode.h"
#include "CMMConfig.h"
#include "CMMMeteTranslate.h"
#include "SysCommon.h"
#include "../../ExtAppIpc/ExtAppIpcApi.h"

//#include "../../NetComm/FSUUtil.h"

namespace CMM_SJY
{
	CData CMMMsg::GetMethod()
	{
		return m_method;
	}

	ISFIT::CXmlElement& CMMMsg::GetInfoNode()
	{
		return m_infoNode;
	}

	int CMMMsg::Prase( char *msg )
	{
		if(m_doc.Parse(msg) < 0)
		{
			return -1;
		}
		return 0;
	}

	ISFIT::CXmlDoc& CMMMsg::GetXmlDoc()
	{
		return m_doc;
	}

	int CMMMsg::Decode(ISFIT::CXmlElement&)
	{
		return -1;
	}

	int CMMMsg::SetResponseXml( CData &xml )
	{
		return -1;
	}

	CMMMsg::~CMMMsg()
	{

	}




	int CMMRequset::Decode(ISFIT::CXmlElement& element)
	{
		m_method = element.GetSubElement(CMM_SJY::PK_Type).GetSubElement(CMM_SJY::Name).GetElementText();
		m_infoNode = element.GetSubElement(CMM_SJY::Info);
		return 0;
	}


	CMMResponse::CMMResponse()
	{
		m_rspBuf = NULL;
		m_bufSize = 0;
	}

	CMMResponse::CMMResponse( char* rspBuf, int size )
	{
		m_rspBuf = rspBuf;
		m_bufSize = size;
	}

	int CMMResponse::Decode(ISFIT::CXmlElement& element)
	{
		m_method = element.GetSubElement(CMM_SJY::PK_Type).GetSubElement(CMM_SJY::Name).GetElementText();
		m_infoNode = element.GetSubElement(CMM_SJY::Info);
		return 0;
	}

	int CMMResponse::SetResponseXml( CData &xml )
	{
		if(xml.length() > m_bufSize)
		{
			LogError("cmm response data is too big");
			return -1;
		}
		strcpy(m_rspBuf, xml.c_str());
		return 0;
	}


	int MsgProcess::OnMsgProcess(char * msg, char *returnBuf, int size )
	{
		ISFIT::CXmlDoc doc;
		if(doc.Parse(msg) < 0)
		{
			return -1;
		}
		ISFIT::CXmlElement element = doc.GetElement(CMM_SJY::Request);
		if(element != NULL)
		{
			return DoRequest(element, returnBuf, size);
		}
		element = doc.GetElement(CMM_SJY::Response);
		if(element != NULL)
		{
			return DoResponse(element);
		}
		return -1;		
	}

	int MsgProcess::OnMsgProcess_Error(char* msg, char* returnBuf, int size, int errorType)
	{
		ISFIT::CXmlDoc doc;
		if (doc.Parse(msg) < 0)
		{
			return -1;
		}
		ISFIT::CXmlElement element = doc.GetElement(CMM_SJY::Request);
		if (element != NULL)
		{
			return DoRequest_Error(element, returnBuf, size, errorType);
		}
		/*element = doc.GetElement(CMM_SJY::Response);
		if (element != NULL)
		{
			return DoResponse(element);
		}*/
		return -1;
	}

	int MsgProcess::DoRequest_Error(ISFIT::CXmlElement& element, char* returnBuf, int size, int errorType)
	{
		CMMRequset request;
		CMMResponse response(returnBuf, size);
		request.Decode(element);
		std::map<CData, processFun>::iterator pos = m_msgMap.find(request.GetMethod());
		if (pos == m_msgMap.end())
		{
			LogError("cmm msg can not process method name:" << request.GetMethod());
			return -1;
		}
		ISFIT::CXmlElement info = request.GetInfoNode();
		CData method = request.GetMethod();
		CData fusId = info.GetSubElement("FSUID").GetElementText();
		CData rsp;
		if (errorType == 6)
		{
			rsp = CMMProtocolEncode::BuildLoginErrorMsg(method);
			response.SetResponseXml(rsp);
		}
		else if (errorType == 3)
		{
			rsp = CMMProtocolEncode::BuildAuthErrorMsg(method);
			response.SetResponseXml(rsp);
		}
		LogInfo("DoRequestError response:" << rsp.c_str());
		return 0;
	}

	int MsgProcess::OnLoginRsp(ISFIT::CXmlElement& info)
	{
		return info.GetSubElement("Result").GetElementText().convertInt();
	}

	MsgProcess::MsgProcess()
	{
		m_msgMap[CMM_SJY::method::GET_DEV_CONF] = &MsgProcess::OnGetDevConf;
		m_msgMap[CMM_SJY::method::GET_FSUINFO] = &MsgProcess::OnGetFSUInfo;
		m_msgMap[CMM_SJY::method::SET_DEV_CONF_DATA] = &MsgProcess::OnSetDevConf;
		m_msgMap[CMM_SJY::method::GET_DATA] = &MsgProcess::OnGetData;
		m_msgMap[CMM_SJY::method::TIME_CHECK] = &MsgProcess::OnTimeCheck;
		m_msgMap[CMM_SJY::method::SET_FSUREBOOT] = &MsgProcess::OnReboot;
		m_msgMap[CMM_SJY::method::SET_POINT] = &MsgProcess::OnSetPoint;
		m_msgMap[CMM_SJY::method::GET_THRESHOLD] = &MsgProcess::OnGetThreshold;
		m_msgMap[CMM_SJY::method::SET_THRESHOLD] = &MsgProcess::OnSetThreshold;
		m_msgMap[CMM_SJY::method::SET_FTP] = &MsgProcess::OnSetFtpInfo;
		m_msgMap[CMM_SJY::method::GET_FTP] = &MsgProcess::OnGetFtpInfo;
		m_msgMap[CMM_SJY::method::SET_LOGININFO] = &MsgProcess::OnSetLoginInfo;
		m_msgMap[CMM_SJY::method::GET_LOGININFO] = &MsgProcess::OnGetLoginInfo;
		m_msgMap[CMM_SJY::method::UPDATE_FSUINFO_INTERVAL] = &MsgProcess::OnUpdateFsuInterval;

		//canyon
		m_msgMap[CMM_SJY::method::GET_STORAGERULE] = &MsgProcess::OnGetStorageRule;
		m_msgMap[CMM_SJY::method::SET_STORAGERULE] = &MsgProcess::OnSetStorageRule;

		//add new 
		m_msgMap[CMM_SJY::method::GET_TIME] = &MsgProcess::OnGetTime;
		m_msgMap[CMM_SJY::method::SET_ACCEPT_IP_CONF] = &MsgProcess::OnSetAcceptIP;
		m_msgMap[CMM_SJY::method::SET_FSUREBOOT] = &MsgProcess::OnSetFsuReboot;
	}

	int MsgProcess::DoRequest(ISFIT::CXmlElement& element, char* returnBuf, int size)
	{
		CMMRequset request;
		CMMResponse response(returnBuf, size);
		request.Decode(element);
		std::map<CData, processFun>::iterator pos = m_msgMap.find(request.GetMethod());
		if(pos == m_msgMap.end())
		{
			LogError("cmm msg can not process method name:"<<request.GetMethod());
			return -1;
		}
		ISFIT::CXmlElement info = request.GetInfoNode();
		CData method = request.GetMethod();
		CData fusId = info.GetSubElement("FSUID").GetElementText();
		int len=strlen(fusId.c_str());
		int len2=strlen(CMMConfig::instance()->GetFsuId().c_str());
		if(((fusId!=CMMConfig::instance()->GetFsuId())
			&&(method.compareNoCase(CMM_SJY::method::SET_LOGININFO) !=0)
			&&(method.compareNoCase(CMM_SJY::method::TIME_CHECK)!=0)) || ((len==0&&len2==0)&&(method.compareNoCase(CMM_SJY::method::SET_LOGININFO) !=0)&&(method.compareNoCase(CMM_SJY::method::TIME_CHECK)!=0)))
		{			
			CData rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::FAILURE, "fsuid invalid", method+"_ACK");
			response.SetResponseXml(rsp);
		}
		else
		{
			processFun fun = pos->second;
			(this->*fun)(request, response);
			LogInfo("=====>> response : "<<returnBuf);
		}
		return 1;
	}

	int MsgProcess::DoResponse(ISFIT::CXmlElement& element)
	{
		CMMResponse response;
		response.Decode(element);
		if(response.GetMethod().compare(CMM_SJY::method::LOGIN_ACK) == 0){
			return OnLoginRsp(response.GetInfoNode());
		}
		return -1;
	}

	int MsgProcess::OnGetDevConf( CMMMsg& request, CMMMsg & response )
	{
#if 0
	 	ISFIT::CXmlElement info = request.GetInfoNode();
		ISFIT::CXmlElement deviceList = info.GetSubElement("DeviceList");
		std::list<CData> devList;
		int index = 0;
		ISFIT::CXmlElement device = deviceList.GetSubElement("Device", index++);
		CData deviceId;
		while(device != NULL)
		{
			deviceId = device.GetAttribute("ID");
			if(deviceId.length() == 0)
			{
				devList.clear();
				break;
			}
			devList.push_back(deviceId);
			device = deviceList.GetSubElement("Device", index++);
		}
		CData rsp = CMMProtocolEncode::GetDevConf(devList);
		response.SetResponseXml(rsp);
#else
		ISFIT::CXmlElement info = request.GetInfoNode();
		ISFIT::CXmlElement deviceList = info.GetSubElement("DeviceID");
		std::list<CData> devList;		
		CData deviceId = deviceList.GetElementText().convertString();
		devList.push_back(deviceId);
		CData rsp = CMMProtocolEncode::GetDevConf(devList);
		response.SetResponseXml(rsp);
#endif
		return 0;
	}

	int MsgProcess::OnGetFSUInfo( CMMMsg& request, CMMMsg & response )
	{
		CData rsp = CMMProtocolEncode::GetFsuInfo();
		response.SetResponseXml(rsp);
		CMMAccess::instance()->OnHeartBeat();
		return 0;
	}

	int MsgProcess::OnSetDevConf( CMMMsg& request, CMMMsg & response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		//ISFIT::CXmlElement deviceList = info.GetSubElement("Values").GetSubElement("DeviceList");
		ISFIT::CXmlElement deviceList = info.GetSubElement("Values");//无deviceList
		std::map<CData, TDevConf> devMap;
		CProtocolDecode::DecodeDevCfg(deviceList, devMap);
		
		std::list<CData> success, failed;
		CData failedCause="NULL";	
		int result=0;
		if(devMap.size()==0)
			failedCause="请求体未识别到要配置的设备id";
		else{	
			int ret=CMMConfig::instance()->SetDevCfg(devMap,success,failed);
			result=1;
			if(ret<0)
			{
				result=0;
				failedCause="未匹配上对应的设备id";
			}
		}
	/*	std::map<CData, TDevConf>::iterator pos = devMap.begin();
		while(pos != devMap.end())
		{
			success.push_back(pos->first);
			pos++;
		}*/
		CData rsp = CMMProtocolEncode::GenGeneralSetRsp(result,failedCause,CMM_SJY::method::SET_DEV_CONF_DATA_ACK, success, failed);
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnGetData( CMMMsg& request, CMMMsg & response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		ISFIT::CXmlElement deviceList = info.GetSubElement("DeviceList");
		
		std::map<CData, std::list<TSemaphore> > reqDevMap;
		CProtocolDecode::DecodeGetDeviceList(deviceList, reqDevMap);
		
		std::map<CData, std::list<TSemaphore> > rspDevMap;
		bool bOK=true;
	
		if (0 == reqDevMap.size())
		{
			std::list <CData> devIdList;
			APPAPI::GetDevId("alias", devIdList);
			LogInfo("=== reqDevMap size == 0 ==, devIdList.size():"<<devIdList.size());
			for (auto it=devIdList.begin(); it!=devIdList.end(); it++)
			{
				CData aliasDevId = *it;
				std::list<TSemaphore> rspSemaphoreList;
		     	LogInfo("--------AliasDevid:"<<aliasDevId);

				std::set<CData> attrSet;
				attrSet.insert("meterId");
				attrSet.insert("val");
				attrSet.insert("time");
				attrSet.insert("setupVal");
				attrSet.insert("alarmLevel");
				attrSet.insert("meterType");
				
				std::list<std::map<CData,CData> > paramList;
				APPAPI::GetMeterInfo(aliasDevId, "alias",attrSet, paramList,5000);
				for (auto mit=paramList.begin(); mit!=paramList.end(); mit++)
				{
					std::map<CData,CData>& attr = *mit;		

					CData meterId=attr["meterId"];		
					//LogInfo("--------meterId:"<<meterId);
					int len=meterId.length();
					CData id=meterId.substr(0,3);
					int iID=id.convertInt();
					if(len>3&&Is_rangeData(iID))
					{							
						TSemaphore rspSemaphore;
						int len=meterId.length();
						rspSemaphore.ID =meterId.substr(0,len-3);
						rspSemaphore.SignalNumber = meterId.substr(len-3,3).convertInt();
						int type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
						//int alarmLevel = attr["alarmLevel"].convertInt();
						int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
						if (nType < 5)
						{
							type = nType;
						}
						else if (nType == 5)
						{
							type = CMM_SJY::ALARM;
						}
						CMMConfig::instance()->SetMeteValues(attr, rspSemaphore, type);
						rspSemaphoreList.push_back(rspSemaphore);
					}
				}
				if (rspSemaphoreList.size()>0)
				{
					rspDevMap[aliasDevId] = rspSemaphoreList;
				}
			}
		}
		else
		{
			for (auto it=reqDevMap.begin(); it!=reqDevMap.end(); it++)
			{
				std::list<TSemaphore> rspSemaphoreList;
				
				const CData& devId = it->first;
				std::list<TSemaphore>& reqMeterIdList = it->second;
				if (reqMeterIdList.size() > 0)
				{
					TDevConf cfg = {0};
					int signalNum=1;
					if(CMMConfig::instance()->GetDevConf(devId, cfg) < 0)
					{
						LogError("get dev config failed id:"<<devId);
						//return -1;
						bOK=false;
						continue;
					}						
					for (auto mit=reqMeterIdList.begin(); mit!=reqMeterIdList.end(); mit++)
					{
						TSemaphore& reqMeterId = *mit;						
						char tmp[32]={0};
						sprintf(tmp,"%03d",signalNum);
						CData strSignalNumber=CData(tmp);	
						CData meterId = reqMeterId.ID+strSignalNumber;	
						std::map<CData,CData> attr;
						// APPAPI::GetMeterVal(devId, meterId, "alias",attr);
						// DAHAI
						APPAPI::GetMeterInfo(devId, meterId, "alias", attr);						
						if(attr["meterId"]!="")
						{
							TSemaphore rspSemaphore;
							rspSemaphore.ID = reqMeterId.ID;
							rspSemaphore.SignalNumber = signalNum;
							int type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
							//int alarmLevel = attr["alarmLevel"].convertInt();
							int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
							if (nType < 5)
							{
								type = nType;
							}
							else if (nType == 5)
							{
								type = CMM_SJY::ALARM;
							}
							CMMConfig::instance()->SetMeteValues(attr, rspSemaphore, type);
							rspSemaphoreList.push_back(rspSemaphore);
						}
						else{
							bOK=false;
						}
					}
				}
				else
				{
					std::set<CData> attrSet;
					attrSet.insert("meterId");
					attrSet.insert("val");
					attrSet.insert("time");
					attrSet.insert("setupVal");
					attrSet.insert("alarmLevel");
					attrSet.insert("meterType");
					
					std::list<std::map<CData,CData> > paramList;
					APPAPI::GetMeterInfo(devId, "alias",attrSet, paramList,5000);
					for (auto mit=paramList.begin(); mit!=paramList.end(); mit++)
					{
						std::map<CData,CData>& attr = *mit;
						CData meterId=attr["meterId"];
							
						int len=meterId.length();
						CData id=meterId.substr(0,3);
						int iID=id.convertInt();
						if(len>3&&Is_rangeData(iID))
						{								
							TSemaphore rspSemaphore;
							int len=meterId.length();
							rspSemaphore.ID =meterId.substr(0,len-3);
							rspSemaphore.SignalNumber = meterId.substr(len-3,3).convertInt();
							int type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
							//int alarmLevel = attr["alarmLevel"].convertInt();
							int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
							if (nType < 5)
							{
								type = nType;
							}
							else if (nType == 5)
							{
								type = CMM_SJY::ALARM;
							}
							CMMConfig::instance()->SetMeteValues(attr, rspSemaphore, type);
							rspSemaphoreList.push_back(rspSemaphore);
						}
					}
				}
				if (rspSemaphoreList.size()>0)
				{
					rspDevMap[devId] = rspSemaphoreList;
				}
			}
		}
		int nResult = bOK ? CMM_SJY::SUCCESS : CMM_SJY::FAILURE;
		if (rspDevMap.size() == 0)
			nResult = CMM_SJY::NODATA;
		CData rsp = CMMProtocolEncode::BuildGetDataRsp(nResult, rspDevMap);
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnTimeCheck( CMMMsg& request, CMMMsg & response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		TTime time = {0};
		CProtocolDecode::DecodeTimeCheck(info, time);
		//Poco::DateTime dateTime(time.Years, time.Month, time.Day, time.Hour, time.Minute, time.Second);
		//CData localTime(DateTimeFormatter::format(dateTime, DateTimeFormat::SORTABLE_FORMAT));
		//dateTime.makeUTC(3600*8);
		//CData utcTime(DateTimeFormatter::format(dateTime, DateTimeFormat::SORTABLE_FORMAT));


		int ret = -1;
		LogInfo("--------OnTimeCheck:"<<time.Years<<"-"<<time.Month<<"-"<<time.Day<<" "
				<<time.Hour<<":"<<time.Minute<<":"<<time.Second);
		if(time.Years<1970||time.Years>2121||time.Month<1||time.Month>12||time.Day<1||time.Day>31 ||time.Hour<0||time.Hour>23 ||time.Minute<0||time.Minute>59||time.Second<0||time.Second>59)
		{
			ret=-1;
			LogInfo("--------time error-------");
		}
		else
		{	
			//1970-01-01 12:00:01
			char str[100] = {0};
			sprintf(str, "%d-%02d-%02d %02d:%02d:%02d:",time.Years,time.Month,time.Day,time.Hour,time.Minute,time.Second);
			CData localTime(str);
			LogInfo("--------set time:"<<localTime);
			ret =APPAPI::SetTime(localTime);
		}
		
		CData rsp;
		if(ret>=0)
			rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::SUCCESS, "NULL", CMM_SJY::method::TIME_CHECK_ACK);
		else
			rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::FAILURE, "字段错误或时间错误", CMM_SJY::method::TIME_CHECK_ACK);		
		
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnReboot( CMMMsg& request, CMMMsg & response )
	{
		CData rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::SUCCESS, "NULL", CMM_SJY::method::SET_FSUREBOOT_ACK);
		response.SetResponseXml(rsp);	
		LogNotice("===MsgProcess::OnReboot()===");
		APPAPI::RebootSys();
		return 0;
	}

	int MsgProcess::OnSetPoint( CMMMsg& request, CMMMsg & response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		ISFIT::CXmlElement value = info.GetSubElement("Value");
		ISFIT::CXmlElement deviceList = value.GetSubElement("DeviceList");
		std::map<CData, std::list<TSemaphore> > devMap;
		CProtocolDecode::DecodeSetPoint(deviceList, devMap);
		std::map<CData, std::list<TSemaphore> >::iterator pos = devMap.begin();
		//bool bFault = false;
		bool bOK=true;
		CData failedCause="NULL";
		if(pos == devMap.end())
		{
			failedCause="请求体未识别到设备id";
		}
		while(pos != devMap.end())
		{
			CData deviceId = pos->first;
			std::list<TSemaphore>::iterator semPos = pos->second.begin();
			while(semPos != pos->second.end())
			{
				int ret = CMMConfig::instance()->SetSemaphoreConf(deviceId, *semPos);
				if (ret<0)
				{
					bOK=false;
					if (ret == -2)
						failedCause="设置值不符合范围";
					else
						failedCause="未查找到对应设备或id";
					semPos->result=0;
				}
				else{
					semPos->result=1;
				}
				semPos++;
			}
			pos++;
		}
		if (bOK)
		{
			CMMConfig::instance()->UpdateCfgFile();
		}
		CData rsp = CMMProtocolEncode::BuildSetPointRsp(bOK?CMM_SJY::SUCCESS:CMM_SJY::FAILURE,failedCause,CMM_SJY::method::SET_POINT_ACK, devMap);
		response.SetResponseXml(rsp);
		//if(devMap.size() !=0)
		//	CMMAccess::instance()->NotifySendData(devMap);  //写监控数据后 上报操作设备的监控点数据
		return 0;
	}

	bool MsgProcess::ThresholdIdFilter(std::map<CData,CData>& attr)
	{
		CData meterId = attr["meterId"];
		if (meterId.length()<9)
		{
			return false;
		}
		CData meterType = attr["meterType"];
		CData subId = meterId.substr(0, 3);
		//LogInfo("----subId:"<<subId<<" meterType:"<<meterType);
		if (meterType == "DI")
			return true;
		return false;
	}

	int MsgProcess::OnGetThreshold( CMMMsg& request, CMMMsg & response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		ISFIT::CXmlElement deviceList = info.GetSubElement("DeviceList");

		std::map<CData, std::list<TThreshold> > rspDevMap;
		
		std::map<CData, std::list<TThreshold> > reqDevMap;
		CProtocolDecode::DecodeGetDeviceList(deviceList, reqDevMap);	
		bool bOK=true;
		LogInfo("reqDevMap size :"<< reqDevMap.size());
		if (0 == reqDevMap.size())
		{
		
			std::list <CData> devIdList;
			APPAPI::GetDevId("alias", devIdList);
			for (auto it=devIdList.begin(); it!=devIdList.end(); it++)
			{
				CData aliasDevId = *it;
				LogInfo("--------AliasDevid:"<<aliasDevId);

				std::set<CData> attrSet;
				attrSet.insert("meterId");
				//attrSet.insert("absoluteVal");
				//attrSet.insert("relativeVal");
				attrSet.insert("threshold");
				attrSet.insert("alarmLevel");
				attrSet.insert("meterType");
				
				std::list<std::map<CData,CData> > paramList;
				APPAPI::GetMeterInfo(aliasDevId, "alias",attrSet, paramList,5000);

				std::list<TThreshold> rspMeterList;
				for (auto mit=paramList.begin(); mit!=paramList.end(); mit++)
				{
					std::map<CData,CData>& attr = *mit;
					CData meterId = attr["meterId"];
					//LogInfo("----meterId:"<<meterId);
					if (ThresholdIdFilter(attr))
					{	
						//LogInfo("----found fit meterId:"<<meterId);
						TThreshold rspMeter;
						int len=meterId.length();
						rspMeter.ID =meterId.substr(0,len-3);
						rspMeter.SignalNumber = meterId.substr(len-3,3).convertInt();	
						rspMeter.NMAlarmID = CMMConfig::instance()->NMAlarmID(rspMeter.ID);

						int type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
						//int alarmLevel = attr["alarmLevel"].convertInt();
						int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
						if (nType < 5)
						{
							type = nType;
						}
						else if (nType == 5)
						{
							type = CMM_SJY::ALARM;
						}
						CMMConfig::instance()->SetMeteThreshold(attr, rspMeter, type);
						
						rspMeterList.push_back(rspMeter);
					}
				}
				
				if (rspMeterList.size()>0)
				{
					rspDevMap[aliasDevId] = rspMeterList;
				}
			}
		}
		else
		{
			for (auto it=reqDevMap.begin(); it!=reqDevMap.end(); it++)
			{
				std::list<TThreshold> rspMeterList;
				CData devId = it->first;
				std::list<TThreshold>& reqMeterIdList = it->second;				
				if (reqMeterIdList.size() > 0)
				{
					//获取
					TDevConf cfg = { 0 };
					if (CMMConfig::instance()->GetDevConf(devId, cfg) < 0)
					{
						LogError("get dev config failed id:" << devId);
						//return -1;
						bOK = false;
						continue;
					}
					for (auto mit=reqMeterIdList.begin(); mit!=reqMeterIdList.end(); mit++)
					{
						TThreshold& reqMeterId = *mit;
						//std::list<TSignal>::iterator pos = cfg.singals.begin();
						int signalNum = 1;
						char tmp[32]={0};
						sprintf(tmp,"%03d",signalNum);
						CData strSignalNumber=CData(tmp);
						
						CData meterId = reqMeterId.ID+strSignalNumber;
						LogInfo("~~~~~~~devid: " <<devId.c_str() <<"meterID: "<<meterId);
						std::map<CData,CData> attr;
						APPAPI::GetMeterParam(devId, meterId, "alias", attr);
						if (ThresholdIdFilter(attr))
						{
							TThreshold rspMeter;
							rspMeter.ID = reqMeterId.ID;
							rspMeter.SignalNumber = signalNum;
							rspMeter.NMAlarmID= CMMConfig::instance()->NMAlarmID(rspMeter.ID);;
							int type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
							//int alarmLevel = attr["alarmLevel"].convertInt();
							int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
							if (nType < 5)
							{
								type = nType;
							}
							else if (nType == 5)
							{
								type = CMM_SJY::ALARM;
							}
							CMMConfig::instance()->SetMeteThreshold(attr, rspMeter, type);
							rspMeterList.push_back(rspMeter);
						}
						else
						{
							LogError("get Threshold failed meterId:" << meterId.c_str());
							bOK=false;
						}
					}
				}
				else
				{
					LogInfo("GetDev:" << devId);
					std::set<CData> attrSet;
					attrSet.insert("meterId");
					//attrSet.insert("absoluteVal");
					//attrSet.insert("relativeVal");
					attrSet.insert("threshold");
					attrSet.insert("alarmLevel");
					attrSet.insert("meterType");
					std::list<std::map<CData,CData> > paramList;
					APPAPI::GetMeterInfo(devId, "alias", attrSet, paramList,5000);
				
					for (auto mit=paramList.begin(); mit!=paramList.end(); mit++)
					{
						std::map<CData,CData>& attr = *mit;
						CData meterId =attr["meterId"];
						
						if (ThresholdIdFilter(attr))
						{							
							TThreshold rspMeter;

							int len=meterId.length();
							rspMeter.ID =meterId.substr(0,len-3);
							rspMeter.SignalNumber = meterId.substr(len-3,3).convertInt();						
							rspMeter.NMAlarmID = CMMConfig::instance()->NMAlarmID(rspMeter.ID);
							int type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
							//int alarmLevel = attr["alarmLevel"].convertInt();
							int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
							if (nType < 5)
							{
								type = nType;
							}
							else if (nType == 5)
							{
								type = CMM_SJY::ALARM;
							}
							CMMConfig::instance()->SetMeteThreshold(attr, rspMeter, type);
							rspMeterList.push_back(rspMeter);
						}
					}
				}
				if (rspMeterList.size()>0)
				{
					rspDevMap[devId] = rspMeterList;
				}
			}
		}
		
		CData rsp = CMMProtocolEncode::BuildGetThresholdRsp(bOK?CMM_SJY::SUCCESS:CMM_SJY::FAILURE, rspDevMap);
		response.SetResponseXml(rsp);
		return 0;
	}
	

	int MsgProcess::OnSetThreshold( CMMMsg& request, CMMMsg & response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		std::map<CData, std::list<TThreshold> > devMap;
		CProtocolDecode::DecodeSetThreshold(info, devMap);
		std::map<CData, std::list<TThreshold> >::iterator pos = devMap.begin();
		//bool bFault = false;
		bool bOK=false;
		CData failedCause="NULL";
		if(pos == devMap.end())
		{
			failedCause="请求体未识别到设备id";
		}

		while(pos != devMap.end())
		{
		/*	if(bFault)
			{
				break;
			}*/
			std::list<TThreshold>::iterator subPos = pos->second.begin();
			while(subPos != pos->second.end())
			{
				LogInfo("DecodeSetThreshold devID : "<< pos->first.c_str());
				if (subPos->Type != CMM_SJY::DI)
				{
					bOK = false;
					failedCause = "该类型量无法设置门限值";
					subPos->result = 0;
					subPos++;
					LogInfo("DecodeSetThreshold 该类型量无法设置门限值 : " << subPos->Type);
					continue;
				}
				if(CMMConfig::instance()->SetThresholdConf(pos->first, *subPos) < 0)
				{
					//bFault = true;
					//break;
					bOK=false;
					failedCause="未查找到对应设备或id";
					subPos->result=0;
				}
				else{
					bOK=true;
					subPos->result=1;
				}
				subPos++;
			}
			pos++;
		}
		
		if(bOK)
		{
			CMMConfig::instance()->UpdateCfgFile();
		}

		CData rsp = CMMProtocolEncode::BuildSetPointRsp(bOK?CMM_SJY::SUCCESS:CMM_SJY::FAILURE,failedCause,CMM_SJY::method::SET_THRESHOLD_ACK, devMap);
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnGetFtpInfo( CMMMsg& request, CMMMsg &response )
	{
		CData rsp = CMMProtocolEncode::BuildGetFtpInfoRsp();
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnSetFtpInfo( CMMMsg& request, CMMMsg& response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		CData user = info.GetSubElement("UserName").GetElementText().convertString();
		CData password = info.GetSubElement("PassWord").GetElementText().convertString();
	
		CMMAccess::instance()->AddLinuxSysUser(user, "", "/");
		int ret= CMMAccess::instance()->ModifyLinuxSysPasswd(user, password);
		CData rsp ;
		if(ret==0)
		{
			//FSUUTIL::SetFtpUser(user, password);
			CMMConfig::instance()->SetFtpUsr(user, true);
			CMMConfig::instance()->SetFtpPasswd(password, true);
			rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::SUCCESS, "NULL", CMM_SJY::method::SET_FTP_ACK);
		}
		else
		{
			rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::FAILURE, "NULL", CMM_SJY::method::SET_FTP_ACK);
		}
  	
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnSetLoginInfo( CMMMsg& request, CMMMsg& response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		CData user = info.GetSubElement("UserName").GetElementText().convertString();
		CData password = info.GetSubElement("PassWord").GetElementText().convertString();
		CData rsp;
		if(password.length() == 0)
		{
			rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::FAILURE, "error", CMM_SJY::method::SET_LOGININFO_ACK);
		}
		else
		{
			CMMConfig::instance()->SetUserName(user,true);
			CMMConfig::instance()->SetPassword(password,true);
			rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::SUCCESS, "NULL", CMM_SJY::method::SET_LOGININFO_ACK);
		}
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnGetLoginInfo( CMMMsg& request, CMMMsg& response )
	{
		CData rsp = CMMProtocolEncode::BuildGetLoginInfoRsp();
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnUpdateFsuInterval( CMMMsg& request, CMMMsg& response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		CData interval = info.GetSubElement("Interval").GetElementText().convertString();	
		CMMAccess::instance()->UpdateInterval(interval);
		CData rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::SUCCESS,"NULL", CMM_SJY::method::UPDATE_FSUINFO_INTERVAL_ACK);
		response.SetResponseXml(rsp);
		return 0;
	}

	
	//canyon add
	int MsgProcess::OnGetStorageRule( CMMMsg& request, CMMMsg & response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		ISFIT::CXmlElement deviceList = info.GetSubElement("DeviceList");


		std::map<CData, std::list<TSignal> > rspDevMap;
		
		std::map<CData, std::list<TSignal> > reqDevMap;
		CProtocolDecode::DecodeGetStorageRuleList(deviceList, reqDevMap);
		bool bOK=true;
		LogInfo("reqDevMap size :" << reqDevMap.size());
		if (0 == reqDevMap.size())
		{
			std::list <CData> devIdList;
			APPAPI::GetDevId("alias", devIdList);
			for (auto it=devIdList.begin(); it!=devIdList.end(); it++)
			{
				CData aliasDevId = *it;
				std::list<TSignal> rspMeterList;

				std::set<CData> attrSet;
				attrSet.insert("meterId");
				attrSet.insert("absoluteVal");
				attrSet.insert("relativeVal");
				attrSet.insert("storePeriod");
				attrSet.insert("meterType");
				
				std::list<std::map<CData,CData> > paramList;
				APPAPI::GetMeterInfo(aliasDevId, "alias",attrSet, paramList,5000);
				for (auto mit=paramList.begin(); mit!=paramList.end(); mit++)
				{
					std::map<CData,CData>& attr = *mit;
					CData meterId=attr["meterId"];		
					int len=meterId.length();
					CData id=meterId.substr(0,3);
					int iID=id.convertInt();
					if(len>3&&Is_rangeData(iID))
					{	
						TSignal rspMeter;
						int len=meterId.length();
						rspMeter.ID =meterId.substr(0,len-3);
						rspMeter.SignalNumber = meterId.substr(len-3,3).convertInt();
						int type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
						//int alarmLevel = attr["alarmLevel"].convertInt();
						int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
						if (nType < 5)
						{
							type = nType;
						}
						else if (nType == 5)
						{
							type = CMM_SJY::ALARM;
						}
						CMMConfig::instance()->SetMeteStorageRule(attr, rspMeter, type);
						rspMeterList.push_back(rspMeter);
					}
				}
				if (rspMeterList.size()>0)
				{
					rspDevMap[aliasDevId] = rspMeterList;
				}
			}
		}
		else
		{
			for (auto it=reqDevMap.begin(); it!=reqDevMap.end(); it++)
			{
				std::list<TSignal> rspMeterList;
				
				CData devId = it->first;
				std::list<TSignal>& reqMeterIdList = it->second;
				LogInfo("reqMeterIdList size :" << reqMeterIdList.size());
				if (reqMeterIdList.size() > 0)
				{
					TDevConf cfg = { 0 };
					if (CMMConfig::instance()->GetDevConf(devId, cfg) < 0)
					{
						LogError("get dev config failed id:"<<devId);
						//return -1;
						bOK=false;
						continue;
					}
					
					for (auto mit=reqMeterIdList.begin(); mit!=reqMeterIdList.end(); mit++)
					{
						TSignal& reqMeterId = *mit;
						//CData NMAlarmID;
						int signalNum = 1;
						//std::list<TSignal>::iterator pos = cfg.singals.begin();
						/*while (pos != cfg.singals.end())
						{
							if (pos->ID.compare(reqMeterId.ID) == 0)
							{
								NMAlarmID = pos->NMAlarmID;
								signalNum = pos->SignalNumber;
								break;
							}
							pos++;
						}

						if (NMAlarmID.empty() || NMAlarmID.length() == 0)
							continue;*/

						char tmp[32]={0};
						sprintf(tmp,"%03d",signalNum);
						CData strSignalNumber=CData(tmp);
						
						CData meterId = reqMeterId.ID+strSignalNumber;
						//LogInfo("~~~~~~~~~~~~~~devid: " <<devId<<"id:"<<reqMeterId.ID<<"signalNum "<<strSignalNumber<<"meterID: "<<meterId);

						std::map<CData,CData> attr;
						APPAPI::GetMeterParam(devId, meterId, "alias", attr);
						if(attr["meterId"]!="")
						{
							TSignal rspMeter;
							rspMeter.ID = reqMeterId.ID;
							rspMeter.SignalNumber = signalNum;
							int type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
							//int alarmLevel = attr["alarmLevel"].convertInt();
							int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
							if (nType < 5)
							{
								type = nType;
							}
							else if (nType == 5)
							{
								type = CMM_SJY::ALARM;
							}
							CMMConfig::instance()->SetMeteStorageRule(attr, rspMeter, type);
							rspMeterList.push_back(rspMeter);
						}
						else
						{
							bOK=false;
						}
					}
				}
				else
				{
					LogInfo("GetDev:" << devId);
					std::set<CData> attrSet;
					attrSet.insert("meterId");
					attrSet.insert("absoluteVal");
					attrSet.insert("relativeVal");
					attrSet.insert("storePeriod");
					attrSet.insert("meterType");
					
					std::list<std::map<CData,CData> > paramList;
					APPAPI::GetMeterInfo(devId, "alias",attrSet, paramList,5000);
					for (auto mit=paramList.begin(); mit!=paramList.end(); mit++)
					{
						std::map<CData,CData>& attr = *mit;
						CData meterId=attr["meterId"];
						int len=meterId.length();
						CData id=meterId.substr(0,3);
						int iID=id.convertInt();
						if(len>3&&Is_rangeData(iID))
						{	
							TSignal rspMeter;
							int len=meterId.length();
							rspMeter.ID =meterId.substr(0,len-3);
							rspMeter.SignalNumber = meterId.substr(len-3,3).convertInt();		
							int type = CMeteTranslate::Instance()->ConvertToCmmMeterType(attr["meterType"]);
							//int alarmLevel = attr["alarmLevel"].convertInt();
							int nType = meterId.substr(3, 1).convertInt();  //第四位判断类型
							if (nType < 5)
							{
								type = nType;
							}
							else if (nType == 5)
							{
								type = CMM_SJY::ALARM;
							}
							CMMConfig::instance()->SetMeteStorageRule(attr, rspMeter, type);
							rspMeterList.push_back(rspMeter);
						}
					}
				}
				
				if (rspMeterList.size()>0)
				{
					rspDevMap[devId] = rspMeterList;
				}
			}
		}
		
		CData rsp = CMMProtocolEncode::BuildGetStorageRuleRsp(bOK?CMM_SJY::SUCCESS:CMM_SJY::FAILURE, rspDevMap);
		response.SetResponseXml(rsp);
		return 0;
	}
	

	int MsgProcess::OnSetStorageRule( CMMMsg& request, CMMMsg& response )
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		std::map<CData, std::list<TSignal> > devMap;
		CProtocolDecode::DecodeSetStorageRule(info, devMap);
		std::map<CData, std::list<TSignal> >::iterator pos = devMap.begin();
		//bool bFault = false;
		bool bOK=false;
		CData failedCause="NULL";
		if(pos == devMap.end())
		{
			failedCause="请求体未识别到设备id";
		}
		
		while(pos != devMap.end())
		{
		/*	if(bFault)
			{
				break;
			}*/
			std::list<TSignal>::iterator subPos = pos->second.begin();
			while(subPos != pos->second.end())
			{
				if(CMMConfig::instance()->SetStorageRuleConf(pos->first, *subPos) < 0)
				{
					//bFault = true;
					//break;
					bOK=false;
					failedCause="未查找到对应设备或id";
					subPos->result=0;
				}
				else
				{
					bOK=true;
					subPos->result=1;
				}
				subPos++;
			}
			pos++;
		}
	//	CMMConfig::instance()->SaveFile();
		if(bOK)
		{
			CMMConfig::instance()->UpdateCfgFile();
		}
		CData rsp = CMMProtocolEncode::BuildSetPointRsp(bOK?CMM_SJY::SUCCESS:CMM_SJY::FAILURE,failedCause,CMM_SJY::method::SET_STORAGERULE_ACK, devMap);
		response.SetResponseXml(rsp);
		return 0;
	}
	//add end

	//NEW
	int MsgProcess::OnGetTime(CMMMsg& request, CMMMsg& response)
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		CData rsp = CMMProtocolEncode::BuildGetTimeRsp(CMM_SJY::SUCCESS, "NULL", CMM_SJY::method::GET_TIME_ACK);
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnSetAcceptIP(CMMMsg& request, CMMMsg& response)
	{
		ISFIT::CXmlElement info = request.GetInfoNode();
		ISFIT::CXmlElement acceptList = info.GetSubElement("AcceptList");

		int idIndex = 0;
		ISFIT::CXmlElement list = acceptList.GetSubElement("List", idIndex);
		while (list != NULL)
		{
			CData familyType = list.GetAttribute("Type");
			std::list<CData> ipList;
			int idIndex2 = 0;
			ISFIT::CXmlElement IP = list.GetSubElement("IP", idIndex2);
			while (IP != NULL)
			{
				CData ip = IP.GetElementText();
				if (ip.length() == 0) {
					continue;
				}
				ipList.push_back(ip);
				IP = list.GetSubElement("IP", idIndex2++);
			}
			list = acceptList.GetSubElement("List", idIndex++);
			CMMConfig::instance()->addAcceptIP(familyType, ipList);
		}
		CData rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::SUCCESS, "NULL", CMM_SJY::method::SET_ACCEPT_IP_CONF_ACK);
		response.SetResponseXml(rsp);
		return 0;
	}

	int MsgProcess::OnSetFsuReboot(CMMMsg& request, CMMMsg& response)
	{
		CData rsp = CMMProtocolEncode::BuildSetLoginRsp(CMM_SJY::SUCCESS, "NULL", CMM_SJY::method::SET_FSUREBOOT_ACK);
		response.SetResponseXml(rsp);
		APPAPI::RebootSys();
		return 0;
	}
}



