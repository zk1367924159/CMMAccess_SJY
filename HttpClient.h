//canyon 2019 09 06

#pragma once

#include <stdio.h>
#include "Poco/Net/SocketAddress.h"
#include "Poco/SharedPtr.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Timestamp.h"
#include "Data.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/URI.h"
#include "Poco/DigestStream.h"

using namespace Poco::Net;


namespace CMM_SJY
{

	class CHttpClient :public HTTPClientSession
	{

	public:
		CHttpClient();
		~CHttpClient();
		void Start();
		/*
		* ����xml���� ����1�ɹ� -2��ʱ ����ʧ��
		*/
		int SendXmlData(const char* url, CData xmlData, CData authHeader, HTTPResponse& response, CData& responseData);

		/*
		* ��ȡresponse ��֤ͷ token
		*/
		static CData GetTokenFromHeader(const HTTPResponse& response);

	private:
		std::string m_pUser;
		//Context::Ptr m_pContext;
	};
		
}
