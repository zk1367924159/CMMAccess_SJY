#ifndef _CMMMSGPROCESS_H
#define _CMMMSGPROCESS_H
#include "NetComm/CXmlElement.h"
#include "CMMCommonStruct.h"
#include "Data.h"
#include "map"


namespace CMM_SJY
{	
	class CMMMsg
	{
	protected:
		ISFIT::CXmlDoc m_doc;
		ISFIT::CXmlElement m_infoNode;
		CData m_method;
	public:
		virtual ~CMMMsg();
		CData GetMethod();
		ISFIT::CXmlDoc& GetXmlDoc();
		ISFIT::CXmlElement& GetInfoNode();
		int Prase(char *msg);
		virtual int Decode(ISFIT::CXmlElement&);
		virtual int SetResponseXml(CData &xml);
		
	};


	class CMMRequset:public CMMMsg
	{
	public:
		virtual int Decode(ISFIT::CXmlElement& element);

	};


	class CMMResponse:public CMMMsg
	{
	private:
		char* m_rspBuf;
		int m_bufSize;
	public:
		CMMResponse();
		CMMResponse(char* rspBuf, int size);
		virtual int Decode(ISFIT::CXmlElement& element);
		virtual int SetResponseXml(CData &xml);
	};

	class MsgProcess
	{
		typedef int (MsgProcess::*processFun)(CMMMsg& request, CMMMsg & response);
	private:
		std::map<CData, processFun> m_msgMap;
	public:
		MsgProcess();
		int OnMsgProcess(char * msg, char *response, int size);
		int OnMsgProcess_Error(char* msg, char* returnBuf, int size, int errorType);
	private:
		int DoRequest(ISFIT::CXmlElement& element, char* returnBuf, int size);
		int DoResponse(ISFIT::CXmlElement& element);
		int OnLoginRsp(ISFIT::CXmlElement& info);
		int OnGetDevConf(CMMMsg& request, CMMMsg & response);
		int OnGetFSUInfo(CMMMsg& request, CMMMsg & response);
		int OnSetDevConf(CMMMsg& request, CMMMsg & response);
		int OnGetData(CMMMsg& request, CMMMsg & response);
		int OnSetPoint(CMMMsg& request, CMMMsg & response);
		int OnTimeCheck(CMMMsg& request, CMMMsg & response);
		int OnReboot(CMMMsg& request, CMMMsg & response);
		int OnGetThreshold(CMMMsg& request, CMMMsg & response);
		int OnSetThreshold(CMMMsg& request, CMMMsg & response);
		int OnGetFtpInfo(CMMMsg& request, CMMMsg &response);
		int OnSetFtpInfo(CMMMsg& request, CMMMsg& response);
		int OnSetLoginInfo(CMMMsg& request, CMMMsg& response);
		int OnGetLoginInfo(CMMMsg& request, CMMMsg& response);
		int OnSetStorageRule( CMMMsg& request, CMMMsg& response );
		int OnGetStorageRule( CMMMsg& request, CMMMsg& response );
		int OnUpdateFsuInterval(CMMMsg& request, CMMMsg& response);

		//new
		int OnGetTime(CMMMsg& request, CMMMsg& response);
		int OnSetAcceptIP(CMMMsg& request, CMMMsg& response);
		int OnSetFsuReboot(CMMMsg& request, CMMMsg& response);

		int DoRequest_Error(ISFIT::CXmlElement& element, char* returnBuf, int size, int errorType);
	private:
		bool ThresholdIdFilter(std::map<CData,CData>& attr);
	};
}
#endif
