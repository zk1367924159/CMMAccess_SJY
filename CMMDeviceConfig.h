#ifndef _CMMDeviceConfig_H
#define _CMMDeviceConfig_H
#include "Data.h"
#include "CMMCommonStruct.h"
#include "SmartLock.h"

namespace CMM_SJY{
	class CMMDeviceConfig{
	public:
	
		std::map<CData, TDeviceInfo> m_aliasId2Info;  //设备ID---》设备信息	
		std::string m_jsonData; //最新设备列表json文本
		ISFIT::CSmartMutex m_devCfgMutex;
		static CMMDeviceConfig* _instance;
	public:
		static CMMDeviceConfig* instance();
		int Init();
		std::vector<int> vStringSplit(const CData& s, const std::string& delim=",");
		/*
		* 首次启动读取文件到map
		*/
		bool ReadDeviceFileConfig();
		/*
		* 获取所有设备列表map
		*/
		std::map<CData, TDeviceInfo> &GetDevices();
		/*
		* 获取指定设备列表结构
		*/
		int GetDevConf(CData devid, TDeviceInfo& cfg);
		/*
		* 获取所有设备列表json
		*/
		std::string GetDevJson();
		/*
		* 获取指定设备列表json
		*/
		std::string GetDevJson(CData devID);
		/*
		* 设备单个设备信息json
		*/
		int SetDevConf(std::string& jsonData);
		bool ParseJson(std::string& jsonData);
		/*
		*将设备列表json导入为map
		*/
		bool ParseJson2Map(std::string& jsonData);
		/*
		*将json导入为设备列表map
		*/
		void ParseMap2Json();
		/*
		* 保存json到文件
		*/
		void SaveFile();
		/*
		* http响应json
		*/
		std::string EncodeResponseJson(int nType);
		/*
		* http响应json
		*/
		std::string DecodeResponseJson(int nType);
	
	};
}
#endif
