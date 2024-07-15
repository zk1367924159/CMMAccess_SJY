#ifndef _CMMPROTOCOLENCODE_H
#define _CMMPROTOCOLENCODE_H
#include "Data.h"
#include "NetComm/CXmlElement.h"
#include "CMMCommonStruct.h"
#include "CMMConfig.h"
#include "CLog.h"
namespace CMM_SJY
{
#define CMM_REQUEST_XML_HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<Request><PK_Type><Name></Name></PK_Type></Request>"

#define CMM_RESPONSE_XML_HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<Response><PK_Type><Name></Name></PK_Type></Response>"

	class CMMProtocolEncode
	{
	public:
		static CData BuildLogMsg();
		static CData ReportDevConf();
		static CData GetDevConf(std::list<CData>& devList);
		static CData GetFsuInfo();
		static void AddDevicesInfo(ISFIT::CXmlElement& devList);
		static void AddDevicesInfo(ISFIT::CXmlElement& devList, std::list<CData> &idList);
		static CData GenGeneralSetRsp(int result,CData FailureCause,CData type, std::list<CData> scucessList, std::list<CData> failList);
		static CData BuildGetDataRsp(int result, std::map<CData, std::list<TSemaphore> > &devMap);
		static CData BuildGetStorageRuleRsp(int result,  std::map<CData, std::list<TSignal> > &devMap );
		static CData BuildGetThresholdRsp(int result, std::map<CData, std::list<TThreshold> >&devMap);			
		static CData BuildAlarmReportInfo(std::list<TAlarm>&alarmList);
		static CData BuildDataReport(std::map<CData, std::list<TSemaphore>>& mapSem);
		static CData BuildGetFtpInfoRsp();
		static CData BuildSetLoginRsp(int result, CData reason, CData type);
		static CData BuildGeneralRsp(int result, CData reason, CData type);
		static CData BuildGetLoginInfoRsp();
		static CData BuildGetTimeRsp(int result, CData reason, CData type);
		//static CData BuildSetPointRsp(EnumResult result,std::map<CData, std::list<TSemaphore>>& devMap );
		//static CData BuildSetThresholdRsp(EnumResult result, std::map<CData, std::list<TThreshold>>& devMap );
		//static CData BuildSetStorageRuleRsp(EnumResult result,std::map<CData, std::list<TSignal>>& devMap );
		template<class T>
		static CData BuildSetPointRsp(int result,CData failedCause, CData type, std::map<CData, std::list<T> >& devMap)
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
				info.AddSubElement("FailureCause").SetElementText(failedCause.c_str());
				info.AddSubElement("FSUID").SetElementText(CMMConfig::instance()->GetFsuId().c_str());
				ISFIT::CXmlElement deviceList = info.AddSubElement("DeviceList");
				//if(result == CMM_SJY::SUCCESS)
				{
					typename std::map<CData, std::list<T> >::iterator pos = devMap.begin();
					while(pos != devMap.end())
					{
						ISFIT::CXmlElement Device  = deviceList.AddSubElement("Device");
						Device.SetAttribute("ID",pos->first.c_str());
						typename std::list<T>::iterator subPos = pos->second.begin();
						ISFIT::CXmlElement SuccessList = Device.AddSubElement("SuccessList");
						ISFIT::CXmlElement failedList = Device.AddSubElement("FailList");
						while(subPos != pos->second.end())
						{
							ISFIT::CXmlElement idEle;
							if(subPos->result==1)
							{
								idEle  = SuccessList.AddSubElement("TSignalMeasurementId");
								
							}
							else 
							{	  
							   idEle  = failedList.AddSubElement("TSignalMeasurementId");
								
							}
							idEle.SetAttribute("ID",subPos->ID.c_str());
							idEle.SetAttribute("SignalNumber",subPos->SignalNumber);
							
							subPos++;
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
		};


	public:
		static CData BuildAuthErrorMsg(CData type);
		static CData BuildLoginErrorMsg(CData type);
	};
}
#endif
