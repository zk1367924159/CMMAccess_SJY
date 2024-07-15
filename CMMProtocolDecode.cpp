#include "CMMProtocolDecode.h"
namespace CMM_SJY
{

	int CProtocolDecode::DecodeDevCfg( ISFIT::CXmlElement& devices, std::map<CData,TDevConf>& devMap)
	{
		int index = 0;
		ISFIT::CXmlElement Device  = devices.GetSubElement(CMM_SJY::Device, index++);
		while(Device != NULL)
		{
			TDevConf dev = {0};
			dev.DeviceID = Device.GetAttribute("DeviceID");
			dev.BeginRunTime = Device.GetAttribute("BeginRunTime");
			dev.Brand = Device.GetAttribute("Brand");
			dev.DevDescribe = Device.GetAttribute("DevDescribe");
			dev.DeviceName = Device.GetAttribute("DeviceName");
			dev.DeviceSubType = Device.GetAttribute("DeviceSubType");
			dev.DeviceType = Device.GetAttribute("DeviceType");
			dev.Model = Device.GetAttribute("Model");
			dev.RatedCapacity = Device.GetAttribute("RatedCapacity").convertDouble();
			dev.RoomName = Device.GetAttribute("RoomName");
			dev.RoomID = Device.GetAttribute("RoomID");
			dev.SiteName = Device.GetAttribute("SiteName");
			dev.SiteID = Device.GetAttribute("SiteID");
			dev.Version = Device.GetAttribute("Version");
			dev.ConfRemark = Device.GetAttribute("ConfRemark");
			ISFIT::CXmlElement signals = Device.GetSubElement("Signals");
			int singalIndex = 0;
			ISFIT::CXmlElement signal = signals.GetSubElement("Signal", singalIndex++);
			while(signal != NULL)
			{
				TSignal tSignal;
				tSignal.AbsoluteVal = signal.GetAttribute("AbsoluteVal").convertDouble();
				tSignal.AlarmLevel = signal.GetAttribute("AlarmLevel").convertInt();
				tSignal.Describe = signal.GetAttribute("Describe");
				tSignal.ID = signal.GetAttribute("ID");
				tSignal.NMAlarmID = signal.GetAttribute("NMAlarmID");
				tSignal.RelativeVal = signal.GetAttribute("RelativeVal").convertDouble();
				tSignal.SignalName = signal.GetAttribute("SignalName");
				tSignal.SignalNumber = signal.GetAttribute("SignalNumber").convertInt();
				tSignal.Threshold = signal.GetAttribute("Threshold").convertDouble();
				int nType = signal.GetAttribute("Type").convertInt();
				if (nType == CMM_SJY::ALARM)  //接收到的告警归属到DI
					nType = CMM_SJY::DI;
				else if (nType == CMM_SJY::DI)//接收到的DI归属到AI
					nType = CMM_SJY::AI;
				tSignal.Type = nType;
				tSignal.SetupVal = signal.GetAttribute("SetupVal").convertDouble();
				dev.singals.push_back(tSignal);
				signal = signals.GetSubElement("Signal", singalIndex++);
			}
			devMap[dev.DeviceID] = dev;
			Device  = devices.GetSubElement(CMM_SJY::Device, index++);
		}
		return 0;
	}

	int CProtocolDecode::DecodeTimeCheck( ISFIT::CXmlElement& info, TTime& time )
	{
		ISFIT::CXmlElement Time = info.GetSubElement("Time");
		if (Time.isEmpty())
			return -1;
		time.Years = Time.GetSubElement("Year").GetElementText().convertInt();
		time.Month = Time.GetSubElement("Month").GetElementText().convertInt();
		time.Day = Time.GetSubElement("Day").GetElementText().convertInt();
		time.Hour = Time.GetSubElement("Hour").GetElementText().convertInt();
		time.Minute = Time.GetSubElement("Minute").GetElementText().convertInt();
		time.Second = Time.GetSubElement("Second").GetElementText().convertInt();
		return 0;
	}

	int CProtocolDecode::DecodeSetPoint( ISFIT::CXmlElement& info, std::map<CData, std::list<TSemaphore> >& devMap )
	{
		int index = 0;
		ISFIT::CXmlElement device = info.GetSubElement("Device", index++);
		while(device != NULL)
		{
			CData deviceId = device.GetAttribute("ID");
			int subIndex = 0;
			std::list<TSemaphore> idList;
			ISFIT::CXmlElement subElement = device.GetSubElement("TSemaphore", subIndex++);
			while(subElement != NULL)
			{
				TSemaphore semaphore = {0};
				semaphore.ID = subElement.GetAttribute("ID");
				semaphore.SetupVal = subElement.GetAttribute("SetupVal").convertDouble();
				semaphore.SignalNumber = subElement.GetAttribute("SignalNumber").convertInt();
				idList.push_back(semaphore);
				subElement = device.GetSubElement("TSemaphore", subIndex++);
			}
			devMap[deviceId] = idList;
			device = info.GetSubElement("Device", index++);
		}
		return 0;
	}

	int CProtocolDecode::DecodeSetThreshold( ISFIT::CXmlElement&info, std::map<CData, std::list<TThreshold> >& devMap )
	{
		ISFIT::CXmlElement devicelist = info.GetSubElement("Value").GetSubElement("DeviceList");
		if (devicelist.isEmpty())
			return -1;
		int index = 0; 
		ISFIT::CXmlElement device = devicelist.GetSubElement("Device", index++);
		while(device != NULL)
		{
			CData deviceId = device.GetAttribute("ID");
			int subIndex = 0;
			ISFIT::CXmlElement threshold = device.GetSubElement("TThreshold", subIndex++);
			std::list<TThreshold> valueList;
			while(threshold != NULL)
			{
				TThreshold thresholdValue = {0};
				thresholdValue.AbsoluteVal = threshold.GetAttribute("AbsoluteVal").convertDouble();
				thresholdValue.ID = threshold.GetAttribute("ID");
				thresholdValue.RelativeVal = threshold.GetAttribute("RelativeVal").convertDouble();
				thresholdValue.SignalNumber = threshold.GetAttribute("SignalNumber").convertInt();
				thresholdValue.Status = threshold.GetAttribute("Status").convertInt();
				thresholdValue.Threshold = threshold.GetAttribute("Threshold").convertDouble();
				//thresholdValue.Type = threshold.GetAttribute("Type").convertInt();
				int nType = threshold.GetAttribute("Type").convertInt();
				
				if (nType == CMM_SJY::ALARM)
				{
					nType = CMM_SJY::DI;
				}
				/*if (nType != CMM_SJY::DI)
				{
					threshold = device.GetSubElement("TThreshold", subIndex++);
					continue;
				}*/
					
				thresholdValue.Type = nType;
				thresholdValue.NMAlarmID = threshold.GetAttribute("NMAlarmID");
				thresholdValue.AlarmLevel = threshold.GetAttribute("AlarmLevel").convertInt();
				threshold = device.GetSubElement("TThreshold", subIndex++);
				valueList.push_back(thresholdValue);
			}
			devMap[deviceId] = valueList;
			device = devicelist.GetSubElement("Device", index++);
		}
		return 0;
	}

	//canyon
	
	int CProtocolDecode::DecodeSetStorageRule( ISFIT::CXmlElement&info, std::map<CData, std::list<TSignal> >& devMap )
	{
		ISFIT::CXmlElement devicelist = info.GetSubElement("Value").GetSubElement("DeviceList");
		if (devicelist.isEmpty())
			return -1;
		int index = 0; 
		ISFIT::CXmlElement device = devicelist.GetSubElement("Device", index++);
		while(device != NULL)
		{
			CData deviceId = device.GetAttribute("ID");
			int subIndex = 0;
			ISFIT::CXmlElement StorageRule  = device.GetSubElement("TStorageRule", subIndex++);
			std::list<TSignal> valueList;
			while(StorageRule != NULL)
			{
				TSignal val;
				val.ID = StorageRule.GetAttribute("ID");
				val.SignalNumber = StorageRule.GetAttribute("SignalNumber").convertInt();
				val.AbsoluteVal = StorageRule.GetAttribute("AbsoluteVal").convertDouble();
				val.RelativeVal = StorageRule.GetAttribute("RelativeVal").convertDouble();
				
				val.savePeriod = StorageRule.GetAttribute("StorageInterval").convertInt();
				int nType = StorageRule.GetAttribute("Type").convertInt();
				if (nType == CMM_SJY::ALARM)
				{
					nType = CMM_SJY::DI;
				}
				else if (nType == CMM_SJY::DI)
				{
					nType = CMM_SJY::AI;
				}
				val.Type = nType;
				StorageRule = device.GetSubElement("TStorageRule", subIndex++);
				valueList.push_back(val);
			}
			devMap[deviceId] = valueList;
			device = devicelist.GetSubElement("Device", index++);
		}
		return 0;
	}

}



