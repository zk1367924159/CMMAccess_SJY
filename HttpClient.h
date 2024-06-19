//canyon 2019 09 06

#pragma once

#include <stdio.h>
#include "Poco/Net/SocketAddress.h"
#include "Poco/SharedPtr.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Timestamp.h"
#include "Data.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/URI.h"
#include "Poco/DigestStream.h"

using namespace Poco::Net;


namespace CMM_SJY
{

	class CHttpClient :public HTTPSClientSession
	{

	public:
		CHttpClient();
		~CHttpClient();
		void Start();
		/*
		* 发送xml数据 返回1成功 -2超时 其他失败
		*/
		int SendXmlData(const char* url, CData xmlData, CData authHeader, HTTPResponse& response, CData& responseData);

		/*
		* 获取response 认证头 token
		*/
		static CData GetTokenFromHeader(const HTTPResponse& response);

	private:
		std::string m_pUser;
		Context::Ptr m_pContext;
	};
		
}
