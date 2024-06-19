#include "CMMDataLog.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/SharedPtr.h"


#include "CMMMeteTranslate.h"
#include "CMMConfig.h"
#include "SysCommon.h"
#include "CLog.h"


namespace CMM_SJY
{
	CMMDataLog::CMMDataLog()
	{	
		m_index=0;
	}
	
	
	CMMDataLog::~CMMDataLog()
	{

	}

	void CMMDataLog::Save(std::map<CData, CData>& msg)
	{
		CData aliaDevId = msg["aliasDevId"];	
		if(aliaDevId.size()==0)
		{
			//LogInfo("~~~~~~~~~~~~~~Not save ~~~~~~~~~~~~~~aliasDevId is empty");
			return;
		}
		
		Poco::DateTime now;
		CData fileName;
		{
			GenFileName(now, fileName);
			Poco::File file(fileName.c_str());
			if(false == file.exists())
			{
				file.createFile();

				Poco::FileOutputStream fos(fileName.c_str());
				CData tmp;
				//ansi
				//tmp = CData("序号")+","+"采集时间"+","+"DeviceID"+","+"监控点ID"+", SignalNumber,"+"监控点描述"+","+"监控点数据类型"+","+"监控点数据测量值"+"\r\n";
				//utf8
				tmp = CData("序号")+","+"采集时间"+","+"DeviceID"+","+"监控点ID"+", SignalNumber,"+"监控点描述"+","+"监控点数据类型"+","+"监控点数据测量值"+"\r\n";
				fos << tmp.c_str();
				fos.close();
			}
			m_fileName = fileName;
		}

		//LogInfo("~~~~~~~~~~~~~~Test~~~~~~~~~~~~~~~");
		
		{
			char *folder = "/Measurement"; //"AppData/Measurement";
			Poco::File dir(folder);
			if(true == dir.exists())
			{
				//printf("found folder\n");
			}
			else
			{
				dir.createDirectory();
			}
			
			if (dir.isDirectory())
			{
				//printf("%s is directory\n", folder);
			}
			Poco::File::FileSize folderSize = dir.getSize();
			if (folderSize > 5*1024*1024)
			{
				std::vector<std::string> files;
				dir.list(files);
				std::vector<std::string>::reverse_iterator it=files.rbegin();
				for (; it!=files.rend(); it++)
				{
					std::string filename = *it;
					printf("file name:%s\n", filename.data());
				}
				it=files.rbegin();
				std::string filename = *it;
				Poco::File file(fileName.c_str());
				file.remove();
				LogInfo("folder size>5M, del the first file");
			}
		}

		//LogInfo("~~~~~~~~~~~~~~Test~~~~~~~~~~~~~~~");
		if (m_fileName.size() > 0)
		{
			//LogInfo("~~~~~~~~~~~~~~Test~~~~~~~~~~~~~~~");
			Poco::FileInputStream fis(m_fileName.c_str());

			Poco::File file(m_fileName.c_str());
			Poco::File::FileSize fileSize = file.getSize();

			fileSize += 1024;
			Poco::SharedPtr<char> rdBuf = new char[fileSize]();
			fis.read(rdBuf.get(), fileSize);
			

			//fis>>buf;  // >> 会丢弃遇到的空白，回车，换行符等 

			int serialNo = 1;
			std::string rdstr(rdBuf.get());

			std::size_t found = rdstr.find_last_of("\r\n");
			if (found != std::string::npos)
			{
				found = rdstr.find_last_of("\r\n", found-2);
				if (found != std::string::npos)
				{
					std::size_t start = found;
					found = rdstr.find_first_of(",", start);
					if (found != std::string::npos)
					{
						
						std::size_t end = found;

						std::string sidx(rdstr, start+1, end-start-1);
						std::string test(rdstr, start);
						//LogInfo("~~~~~~~~~~~~~test~~~~~~~~~~~~~~~~start:"<<start<<" end:"<<end<<" test:"<<test<<" sidx:"<<sidx);
						CData idx(sidx);
						serialNo = idx.convertInt() + 1;
						printf("idx:%d\n", serialNo);
					}
				}

			} 
				
			//LogInfo("~~~~~~~~~~~~~~Test~~~~~~~~~~~~~~~");
		
			fis.close();
			
			CData meterId=msg["meterId"];
			int len=meterId.length();
			CData id=meterId.substr(0,3);
			int iID=id.convertInt();
			
			CData meterIdTmp =meterId.substr(0,len-3);
			int type=CMeteTranslate::Instance()->ConvertToCmmMeterType(msg["meterType"]);
			CData strSignalNumber = meterId.substr(len-3,3);			
			 
			if(len>3&&Is_rangeAlarm(iID))
			{	
				Poco::FileOutputStream fos(m_fileName.c_str());
				CData tmp;
				//tmp = CData(serialNo++) + "," + pMeter->GetTime() + "," + pMeter->m_cmmDevId + "," + pMeter->m_CmmId + "," 
				tmp = CData(serialNo++) + "," + msg["time"] + "," + aliaDevId + "," + meterIdTmp + " , "+strSignalNumber +", " +msg["meterName"]
					+ "," + CData((int)type) + ","+msg["val"] + "\r\n";

				CData buf(rdBuf.get());
				buf += tmp;
				fos << buf.c_str();
				fos.close();
			}
			//printf("~~~~~file content:%s\n", buf.c_str());
		}
	}


	void CMMDataLog::GenFileName( Poco::DateTime &date, CData &fileName )
	{
		char buf[256] = {0};
		sprintf(buf,"/Measurement/PM_%s_%04d%02d%02d%02d%02d.csv",CMMConfig::instance()->GetFsuId().c_str(), date.year(), date.month(), date.day(), date.hour(), 0); //date.minute());
		fileName = buf;
	}

}