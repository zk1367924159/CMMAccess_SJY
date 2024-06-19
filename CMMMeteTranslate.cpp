#include "CMMMeteTranslate.h"
#include "NetComm/CSqliteObj.h"
#include "MeteInfo.h"
#include "CMMCommonStruct.h"
#include <stdio.h>

namespace CMM_SJY
{
	CMeteTranslate* CMeteTranslate::_instance = NULL;
	CMeteTranslate* CMeteTranslate::Instance()
	{
		if(_instance == NULL)
		{
			_instance = new CMeteTranslate();
		}
		return _instance;
	}

	void CMeteTranslate::Init()
	{
		ISFIT::CSqliteObj sqlite;
		CData sql = "select * from cmm_mete";
		sqlite.Query(sql);
		int count = sqlite.GetRecodCount();
		ISFIT::CSqlQueryResult record;
		for(int i =0; i < count; i++)
		{
			record = sqlite.GetRecord(i);
			m_metesMap[record.GetValue("innerId")] = record.GetValue("cmmId");
		}
	}

	CData CMeteTranslate::FromInnerToCMM(int id, int &SignalNumber)
	{
		//canyon

		int cmmId = id / 1000;
		int signalNum = id % 1000;
		char cid[10] = {0};
		sprintf(cid, "%06d",cmmId);
		CData meterId(cid);
		SignalNumber = signalNum; 
		return meterId;

		/*
		CData innerId = CData(id);
		SignalNumber = innerId.substr(7,2).convertInt();
		innerId = innerId.substr(0,7)+"01";
		return FromInnerToCMM(innerId);*/
	}

	CData CMeteTranslate::FromInnerToCMM( CData id )
	{
		std::map<CData, CData>::iterator pos = m_metesMap.find(id);
		if(pos == m_metesMap.end())
		{
			return CData("");
		}
		return pos->second;
	}

	CData CMeteTranslate::FromCMMToInner( CData cmmId, int signalNumber )
	{
		CData innerId(cmmId.convertInt()*1000 + signalNumber);
		return innerId; 

		/*
		std::map<CData, CData>::iterator pos = m_metesMap.begin();
		while(pos != m_metesMap.end())
		{
			if(id.compare(pos->second) == 0)
			{
				return pos->first;
			}
			pos++;
		}
		return CData("");*/
	}


	int CMeteTranslate::ConvertToCmmMeterType(CData meterType)
	{
		if (meterType == "AI")
		{
			return CMM_SJY::AI;
		}
		if (meterType == "DI")
		{
			return CMM_SJY::DI;
		}
		if (meterType == "DO")
		{
			return CMM_SJY::DO;
		}
		if (meterType == "AO")
		{
			return CMM_SJY::AO;
		}
		return CMM_SJY::AI;
	}

	int CMeteTranslate::FromInnerPortTypeToCMM(int type, int dataType)
	{
		if(type == SMART_DEV_METE_INFO::ALARM)
		{
			return CMM_SJY::DI;
		}
		else
		{
			if(dataType == BUSINESSLAYER_COMMON_DATA::FLOAT){
				if(type == SMART_DEV_METE_INFO::PICK){
					return CMM_SJY::AI;
				}
				else{
					return CMM_SJY::AO;
				}
			}
			else if(dataType == BUSINESSLAYER_COMMON_DATA::ENUM_TYPE){
				if(type == SMART_DEV_METE_INFO::PICK){
					return CMM_SJY::DI;
				}
				else{
					return CMM_SJY::DO;
				}
			}
			else
			{
				return -1;
			}
		}		
	}

}



