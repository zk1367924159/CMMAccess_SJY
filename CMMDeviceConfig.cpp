//canyon 2019 0902


#include "CMMDeviceConfig.h"
#include "CLog.h"
#include "SysCommon.h"
#include "CMMCommonStruct.h"
#include "CMMAccess.h"
#include "Poco/FileStream.h"
#include "Poco/JSON/Parser.h"
#include "Poco/Path.h"
#include "Poco/File.h"
#include "Poco/StreamCopier.h"
#include "fstream"
#include "../../ExtAppIpc/ExtAppIpcApi.h"
#include "../../ExtAppIpc/ExtAppIpcStruct.h"

#define  CMM_DEVICE_JSON_FILE_PATH "/appdata/config/CMMAccess_SJY_device.json"
namespace CMM_SJY{
	CMMDeviceConfig * CMMDeviceConfig::_instance = NULL;

	CMMDeviceConfig* CMMDeviceConfig::instance()
	{
		if(_instance == NULL){
			_instance = new CMMDeviceConfig();
		}
		return _instance;
	}


	std::vector<int> CMMDeviceConfig::vStringSplit(const  CData& s, const std::string& delim)
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
	
	
	int CMMDeviceConfig::Init()
	{
		//读取移动动环+告警相关配置文件
		if (!ReadDeviceFileConfig())
		{
			LogNotice("ReadDeviceList file not exist.");
		}
		return 0;
	}

	bool CMMDeviceConfig::ReadDeviceFileConfig()
	{
		// 确保文件存在  
		if (!Poco::Path(CMM_DEVICE_JSON_FILE_PATH).isFile()) 
		{
			LogError("CMM_DEVICE_JSON_FILE_PATH is not exist.");
			return false;
		}
		try
		{
			// 打开文件并读取内容  
			std::ifstream fileStream(CMM_DEVICE_JSON_FILE_PATH, std::ios::in | std::ios::binary);
			if (!fileStream.good())
			{
				LogError("CMM_DEVICE_JSON_FILE_PATH is exist but open failed.");
				return false;
			}
			std::string jsonData((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
			if (!ParseJson2Map(jsonData))
			{
				return false;
			}
			m_jsonData = jsonData;
		}
		catch (const std::exception&)
		{
			return false;
		}
		return true;
	}

	std::map<CData, TDeviceInfo> &CMMDeviceConfig::GetDevices()
	{
		//防止另外一个地方在修改m_devCfg
		//FastMutex::ScopedLock lock(m_devCfgMutex);//
		return m_aliasId2Info;
	}

	void CMMDeviceConfig::SaveFile()
	{
		// 获取文件路径
		Poco::Path filePath(CMM_DEVICE_JSON_FILE_PATH);

		// 确保目录存在，如果需要
		Poco::File dir(filePath.parent());
		if (!dir.exists()) dir.createDirectories();

		// 使用路径字符串创建FileStream
		Poco::FileStream outputStream(filePath.toString(), std::ios::out | std::ios::trunc);

		// 写入JSON数据
		outputStream << m_jsonData;

		// 关闭文件流，但这通常是不必要的，因为FileStream的析构函数会自动调用
		outputStream.close();
	}


	int CMMDeviceConfig::GetDevConf(CData devId, TDeviceInfo& cfg)
	{
		if (m_aliasId2Info.find(devId) == m_aliasId2Info.end())
		{
			return -1;
		}
		cfg = m_aliasId2Info[devId];
		return 0;
	}

	std::string CMMDeviceConfig::GetDevJson()
	{
		return m_jsonData;
	}

	std::string CMMDeviceConfig::GetDevJson(CData devId)
	{
		std::string resJson;
		TDeviceInfo cfg;
		std::stringstream jsonDataStream;
		if (-1 == GetDevConf(devId, cfg))
		{
			return resJson;
		}
		jsonDataStream << "{"
			<< " \"DeviceNo\": \"" << cfg.DeviceNo.c_str() << "\","
			<< " \"AliasDeviceNo\": \"" << cfg.AliasDeviceNo.c_str() << "\","
			<< " \"DeviceName\": \"" << cfg.DeviceName.c_str() << "\","
			<< " \"AliasDeviceName\": \"" << cfg.AliasDeviceName.c_str() << "\","
			<< " \"ParentDeviceID\": \"" << cfg.ParentDeviceID.c_str() << "\","
			<< " \"DeviceSubType\" : \"" << cfg.DeviceSubType.c_str() << "\","
			<< " \"Brand\" : \"" << cfg.Brand.c_str() << "\","
			<< " \"Model\" : \"" << cfg.Model.c_str() << "\","
			<< " \"Desc\" : \"" << cfg.Desc.c_str() << "\","
			<< " \"RatedCapacity\" : \"" << cfg.RatedCapacity.c_str() << "\","
			<< " \"Version\" : \"" << cfg.Version.c_str() << "\","
			<< " \"BeginRunTime\" : \"" << cfg.BeginRunTime.c_str() << "\""
			<< "}";

		resJson = jsonDataStream.str();
		return resJson;
	}

	//canyon rebuild
	int CMMDeviceConfig::SetDevConf(std::string& jsonData)
	{
		ISFIT::SmartLock lock(m_devCfgMutex);

		if (ParseJson(jsonData))
		{
			ParseMap2Json();
			SaveFile();
			return 0;
		}
		return -1;
	}

	bool CMMDeviceConfig::ParseJson(std::string& jsonData)
	{
		// 解析JSON字符串  
		Poco::JSON::Parser parser{};
		Poco::JSON::Object::Ptr jsonObj;
		try
		{
			Poco::Dynamic::Var val = parser.parse(jsonData);
			jsonObj = val.extract<Poco::JSON::Object::Ptr>();
		}
		catch (const Poco::Exception& e)
		{
			LogNotice("e:" << e.what());
			return false;
		}
		if (!jsonObj)
		{
			LogNotice("jsonObj is nullptr.");
			return false;
		}
		TDeviceInfo sInfo;
		try
		{
			sInfo.DeviceNo = jsonObj->getValue<std::string>("DeviceNo");
			sInfo.AliasDeviceNo = jsonObj->getValue<std::string>("AliasDeviceNo");
			sInfo.DeviceName = jsonObj->getValue<std::string>("DeviceName");
			sInfo.AliasDeviceName = jsonObj->getValue<std::string>("AliasDeviceName");
			sInfo.ParentDeviceID = jsonObj->getValue<std::string>("ParentDeviceID");
			sInfo.DeviceSubType = jsonObj->getValue<std::string>("DeviceSubType");
			sInfo.Brand = jsonObj->getValue<std::string>("Brand");
			sInfo.Model = jsonObj->getValue<std::string>("Model");
			sInfo.Desc = jsonObj->getValue<std::string>("Desc");
			sInfo.RatedCapacity = jsonObj->getValue<std::string>("RatedCapacity");
			sInfo.Version = jsonObj->getValue<std::string>("Version");
			sInfo.BeginRunTime = jsonObj->getValue<std::string>("BeginRunTime");
		}
		catch (const std::exception& e)
		{
			LogNotice("exception : " << e.what());
			return false;
		}

		/*std::map<CData, CData> paramMap;
		int nRet = APPAPI::GetDevParam(sInfo.DeviceNo, "msj", paramMap, 5000);
		for (auto iter = paramMap.begin(); iter != paramMap.end(); ++iter)
		{
			LogInfo("GetDevParam  key:" << iter->first << "  value::" << iter->second);
		}
		paramMap["aliasDevName"] = sInfo.AliasDeviceName;
		paramMap["aliasDevId"] = sInfo.AliasDeviceNo;
		paramMap["devId"] = sInfo.DeviceNo;

		nRet = APPAPI::SetDevParam(sInfo.DeviceNo, "msj", paramMap);
		LogInfo("SetDevParam devId:" << sInfo.DeviceNo << " aliasDevId:" << sInfo.AliasDeviceNo << " and return " << nRet);
		nRet = APPAPI::SetDevParam(sInfo.AliasDeviceNo, "alias", paramMap);
		LogInfo("SetDevParam alias  devId return " << nRet);*/

		if (m_aliasId2Info.find(sInfo.DeviceNo) == m_aliasId2Info.end())
		{
			m_aliasId2Info.emplace(sInfo.DeviceNo, sInfo);
		}
		else
		{
			m_aliasId2Info[sInfo.DeviceNo] = sInfo;
		}
		LogInfo("set map sucess and m_aliasId2Info size " << m_aliasId2Info.size());
		return true;
	}

	bool CMMDeviceConfig::ParseJson2Map(std::string& jsonData)
	{
		// 解析JSON字符串  
		Poco::JSON::Parser parser{};
		Poco::JSON::Object::Ptr jsonObj;
		try
		{
			Poco::Dynamic::Var val = parser.parse(jsonData);
			jsonObj = val.extract<Poco::JSON::Object::Ptr>();
		}
		catch (const Poco::Exception& e)
		{
			LogNotice("e:" << e.what());
			return false;
		}
		if (!jsonObj)
		{
			LogNotice("jsonObj is nullptr.");	
			return false;
		}

		std::map<CData, TDeviceInfo> AliasId2Info;
		Poco::JSON::Array::Ptr DeviceList = jsonObj->getArray("DeviceList");
		if (DeviceList)
		{
			for (size_t i = 0; i < DeviceList->size(); ++i)
			{
				// 提取当前元素为对象指针以便访问其属性
				Poco::JSON::Object::Ptr device = DeviceList->getObject(i);
				TDeviceInfo sInfo;
				if (device)
				{
					// 假设设备对象有"name"和"id"属性，这里只是示例，根据实际JSON结构调整
					try
					{
						sInfo.DeviceNo = device->getValue<std::string>("DeviceNo");
						sInfo.AliasDeviceNo = device->getValue<std::string>("AliasDeviceNo");
						sInfo.DeviceName = device->getValue<std::string>("DeviceName");
						sInfo.AliasDeviceName = device->getValue<std::string>("AliasDeviceName");
						sInfo.ParentDeviceID = device->getValue<std::string>("ParentDeviceID");
						sInfo.DeviceSubType = device->getValue<std::string>("DeviceSubType");
						sInfo.Brand = device->getValue<std::string>("Brand");
						sInfo.Model = device->getValue<std::string>("Model");
						sInfo.Desc = device->getValue<std::string>("Desc");
						sInfo.RatedCapacity = device->getValue<std::string>("RatedCapacity");
						sInfo.Version = device->getValue<std::string>("Version");
						sInfo.BeginRunTime = device->getValue<std::string>("BeginRunTime");
						AliasId2Info.emplace(sInfo.DeviceNo, sInfo);
					}
					catch(const Poco::Exception& e)
					{
						LogNotice("e:" << e.what());
						return false;
					}
				}
				else
				{
					LogNotice("One of the devices in DeviceList is not an object.");
					return false;
				}
			}
		}
		m_aliasId2Info = AliasId2Info; //完全无错误才进行此操作
		LogInfo("set map sucess and m_aliasId2Info size " << m_aliasId2Info.size());
		return true;
	}

	void CMMDeviceConfig::ParseMap2Json()
	{
		auto iter = m_aliasId2Info.begin();
		CData deviceListJson;
		while (iter != m_aliasId2Info.end())
		{
			CData devID  = iter->first;
			CData deviceJson = GetDevJson(devID).c_str();
			if(!deviceJson.empty())
				deviceListJson = deviceListJson + deviceJson + ",";
			iter++;
		}
		deviceListJson = deviceListJson.substr(0, deviceListJson.length()-1);
		std::stringstream jsonDataStream;
		jsonDataStream << "{"
			<< " \"DeviceList\": [" << deviceListJson.c_str() << "]"
			<< "}";

		CData strJson;
		strJson = jsonDataStream.str();
		m_jsonData = strJson.c_str();
		LogInfo("m_jsonData: " << m_jsonData);
	}

	std::string CMMDeviceConfig::EncodeResponseJson(int nType)
	{
		std::stringstream jsonDataStream;
		if (nType == 0)
		{
			jsonDataStream << "{"
				<< "\"code\": 0,"
				<< "\"message\": \"set device success.\""
				<< "}";
		}
		else if (nType == -1)
		{
			jsonDataStream << "{"
				<< "\"code\": -1,"
				<< "\"message\": \"set device failed.\""
				<< "}";
		}
		else if (nType == -2)
		{
			jsonDataStream << "{"
				<< "\"code\": -2,"
				<< "\"message\": \"The request type is incorrect.\""
				<< "}";
		}
		else
		{
			jsonDataStream << "{"
				<< "\"code\": -999,"
				<< "\"message\": \"Unknown reason error.\""
				<< "}";

		}
		std::string jsonString = jsonDataStream.str();
		return jsonString;
	}

	std::string CMMDeviceConfig::DecodeResponseJson(int nType)
	{
		std::stringstream jsonDataStream;
		if (nType == 0)
		{
			jsonDataStream << "{"
				<< "\"code\": 0,"
				<< "\"message\": \"get device success.\""
				<< "}";
		}
		else if (nType == -1)
		{
			jsonDataStream << "{"
				<< "\"code\": -1,"
				<< "\"message\": \"get device failed.\""
				<< "}";
		}
		else if (nType == -2)
		{
			jsonDataStream << "{"
				<< "\"code\": -2,"
				<< "\"message\": \"The request type is incorrect.\""
				<< "}";
		}
		else
		{
			jsonDataStream << "{"
				<< "\"code\": -999,"
				<< "\"message\": \"Unknown reason error.\""
				<< "}";

		}
		std::string jsonString = jsonDataStream.str();
		return jsonString;
	}

}




