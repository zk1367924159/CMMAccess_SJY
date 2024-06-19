
#include "Data.h"
#include "tinyxml.h"
#include "CMMCommonStruct.h"
#include "CMMConfig.h"
#include "CLog.h"
namespace CMM_SJY
{
#define CMM_SOAP_REQUEST_XML_HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<Request><PK_Type><Name></Name></PK_Type></Request>"

#define CMM_SOAP_RESPONSE_XML_HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<Response><PK_Type><Name></Name></PK_Type></Response>"

/*
* 服务端接受响应 返回soap组装格式
*/
#define SERVICE_SOAP_RECV_XML_HEAD "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ns1=\"http://LSCService.chinamobile.com\" xmlns:ns2=\"http://FSUService.chinamobile.com\"><SOAP-ENV:Body SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><ns:invokeResponse><invokeReturn>"
#define SERVICE_SOAP_RECV_XML_END "</invokeReturn></ns:invokeResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>"

/*
* 客户端发送请求 soap组装格式
*/
#define CLIENT_SOAP_SEND_XML_HEAD "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ns1=\"http://LSCService.chinamobile.com\" xmlns:ns2=\"http://FSUService.chinamobile.com\"><SOAP-ENV:Body SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><ns1:invoke><xmlData>"
#define CLIENT_SOAP_SEND_XML_END "</xmlData></ns1:invoke></SOAP-ENV:Body></SOAP-ENV:Envelope>"
class CMMSoapXmllEncode
	{
	public:

		//序列化
		static CData setSoapSerialization(std::string& xmlData,int nType);

		//反序列化
		static CData setSoapDeserialization(CData& SoapxmlData);
	};
}
