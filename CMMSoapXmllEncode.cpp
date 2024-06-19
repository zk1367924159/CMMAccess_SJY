#include "CMMSoapXmllEncode.h"
#include <regex>
namespace CMM_SJY
{

	CData replaceAll(const CData& str, const CData from, const CData to)
	{
		CData result;
		size_t pos = 0;
		while ((pos = str.find(from, pos)) != CDATA_NPOS)        
		{
			result += str.substr(0, pos) + to;
			pos += from.length();
		}
		return result + str.substr(pos);
	}

	std::string replace_all(std::string str, const std::string& from, const std::string& to) 
	{
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) 
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
		return str;
	}

	CData CMMSoapXmllEncode::setSoapSerialization(std::string& xmlData,int nType)
	{
		CData serialXml;
		if(nType == 0)
			serialXml = SERVICE_SOAP_RECV_XML_HEAD;
		else
			serialXml = CLIENT_SOAP_SEND_XML_HEAD;
		// ʹ��������ʽһ�����滻 <
		xmlData = replace_all(xmlData,"<","&lt;");
		xmlData = replace_all(xmlData, ">", "&gt;");
		serialXml += CData(xmlData);
		if (nType == 0)
			serialXml += CData(SERVICE_SOAP_RECV_XML_END);
		else
			serialXml += CData(CLIENT_SOAP_SEND_XML_END);
		LogInfo("serialXml2 :" << serialXml.c_str());
		return serialXml;
	}

	CData CMMSoapXmllEncode::setSoapDeserialization(CData& SoapxmlData)
	{
		CData xmlData;
		TiXmlDocument doc;
		const char* errorMsg = doc.Parse(SoapxmlData.c_str());
		if (errorMsg)
		{
			LogError("SoapxmlData Parse failed: " << errorMsg);
			return "";
		}
		TiXmlElement* root = doc.RootElement();
		if (!root)
		{
			LogError("Failed to parse XML, no root element found." << SoapxmlData.c_str());
			return "";
		}
		// ��ȡBodyԪ��
		TiXmlElement* bodyElement = root->FirstChildElement("SOAP-ENV:Body");
		if (!bodyElement)
		{
			bodyElement = root->FirstChildElement("soapenv:Body");
		}
		if (bodyElement)
		{
			// ��ȡinvokeResponseԪ��
			TiXmlElement* invokeResponseElement = bodyElement->FirstChildElement("tns:invoke");
			if (!invokeResponseElement)
			{
				invokeResponseElement = bodyElement->FirstChildElement("ns:invokeResponse");	
			}
			if (invokeResponseElement)
			{
				// ���������Է��ʻ���invokeResponseԪ�ؼ���������
				// ����invokeReturnԪ��  
				TiXmlElement* invokeReturnElement = invokeResponseElement->FirstChildElement("xmlData");
				if (!invokeReturnElement)
				{
					invokeReturnElement = invokeResponseElement->FirstChildElement("invokeReturn");
				}
				if (!invokeReturnElement)
				{
					LogError("invokeReturn element not found." << SoapxmlData.c_str());
					return "";
				}
				// ��ȡinvokeReturnԪ�ص��ı�����  
				const char* pXmlData = invokeReturnElement->GetText();
				if (pXmlData)
				{
					//LogInfo("xmlData : " << pXmlData);
					xmlData = pXmlData;
				}
				else {
					LogError("No text content found in invokeReturn element." << SoapxmlData.c_str());
				}
			}
			else 
			{
				LogError("invokeResponse element not found in SOAP Body." << SoapxmlData.c_str());
			}
		}
		else
		{
			LogError("SOAP Body element not found." << SoapxmlData.c_str());
		}
		return xmlData;
	}
}