#ifndef _CMMPROTOCOLDECODE_H
#define _CMMPROTOCOLDECODE_H
#include "NetComm/CXmlElement.h"
#include "CMMCommonStruct.h"
namespace CMM_SJY
{
	class CProtocolDecode
	{
	public:
		static int DecodeDevCfg(ISFIT::CXmlElement& devices, std::map<CData,TDevConf>& devMap);
		template<class T>
		static int DecodeGetDeviceList(ISFIT::CXmlElement& devices, std::map<CData,std::list<T> >& devMap)
		{
			int index = 0;
			ISFIT::CXmlElement Device  = devices.GetSubElement(CMM_SJY::Device, index++);
			while(Device != NULL)
			{
				CData deviceId = Device.GetAttribute("ID");
				std::list<T> idList;
				if(deviceId.length() == 0)
				{
					devMap.clear();
					break;
				}			
				int idIndex = 0;
				ISFIT::CXmlElement ID = Device.GetSubElement("ID",idIndex++);
				while(ID != NULL)
				{
					CData id = ID.GetElementText();
					if(id.length() == 0){
						idList.clear();
						break;
					}
					T semaphore;
					semaphore.ID = id;
					semaphore.SignalNumber = ID.GetAttribute("SignalNumber").convertInt();
					idList.push_back(semaphore);
					ID = Device.GetSubElement("ID",idIndex++);
				}
				devMap[deviceId] = idList;
				Device  = devices.GetSubElement(CMM_SJY::Device, index++);
			}
			return 0;
		}
		template<class T>
		static int DecodeGetStorageRuleList(ISFIT::CXmlElement& devices, std::map<CData, std::list<T> >& devMap)
		{
			int index = 0;
			ISFIT::CXmlElement Device = devices.GetSubElement(CMM_SJY::Device, index++);
			while (Device != NULL)
			{
				CData deviceId = Device.GetAttribute("ID");
				std::list<T> idList;
				if (deviceId.length() == 0)
				{
					devMap.clear();
					break;
				}
				int idIndex = 0;
				ISFIT::CXmlElement TSignalMeasurementId = Device.GetSubElement("TSignalMeasurementId", idIndex++);
				while (TSignalMeasurementId != NULL)
				{
					CData id = TSignalMeasurementId.GetAttribute("ID");
					if (id.length() == 0) {
						idList.clear();
						break;
					}
					T semaphore;
					semaphore.ID = id;
					semaphore.SignalNumber = TSignalMeasurementId.GetAttribute("SignalNumber").convertInt();
					idList.push_back(semaphore);
					TSignalMeasurementId = Device.GetSubElement("TSignalMeasurementId", idIndex++);
				}
				devMap[deviceId] = idList;
				Device = devices.GetSubElement(CMM_SJY::Device, index++);
			}
			return 0;
		}
		static int DecodeTimeCheck(ISFIT::CXmlElement& info, TTime& time);
		static int DecodeSetPoint(ISFIT::CXmlElement& info, std::map<CData, std::list<TSemaphore> >& devMap);
		static int DecodeSetThreshold(ISFIT::CXmlElement&info, std::map<CData, std::list<TThreshold> >& devMap);
		static int DecodeSetStorageRule( ISFIT::CXmlElement&info, std::map<CData, std::list<TSignal> >& devMap );
	};
}
#endif
