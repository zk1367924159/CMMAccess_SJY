#include "CMMLogFile.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/DateTime.h"
#include "CMMConfig.h"
namespace CMM_SJY
{
	CMMLogFile::CMMLogFile()
	{
		m_perFileSize = 1024*1024;
		m_curIndex = 1;
		GenDirName(m_curDir);
		Poco::File file(m_curDir.c_str());
		if(file.exists())
		{
			std::vector<std::string> files;
			file.list(files);
			std::vector<std::string>::iterator pos = files.begin();
			int tmp = 0;
			while(pos != files.end())
			{
				sscanf(pos->c_str(),"FSUID_alarm%02d.log", &tmp);
				if(tmp > m_curIndex)
				{
					m_curIndex = tmp;
				}
				pos++;
			}
		}
	}

	int CMMLogFile::SetFileSize( int fileSize )
	{
		m_perFileSize = fileSize;
		return 0;
	}

	int CMMLogFile::log(TAlarm &alarm)
	{
		CData tmpDir;
		GenDirName(tmpDir);
		if(m_curDir.compare(tmpDir) != 0)
		{
			m_curDir = tmpDir;
			m_curIndex = 1;
		}
		char fileName[64] ={0};
		sprintf(fileName, "%s/%s_alarm%02d.log", m_curDir.c_str(),CMMConfig::instance()->GetFsuId().c_str(),m_curIndex);
		Poco::File dir(m_curDir.c_str()) ;
		if(dir.exists() == false)
		{
			dir.createDirectory();
		}
		Poco::File file(fileName);
		if(file.exists() == false)
		{
			file.createFile();
		}
		CData cmmAlarmStr;
		FormatAlarmInfo(alarm, cmmAlarmStr);
		if((file.getSize()+cmmAlarmStr.length()) >= m_perFileSize)
		{
			m_curIndex++;
			sprintf(fileName, "%s/%s_alarm%02d.log", m_curDir.c_str(),CMMConfig::instance()->GetFsuId().c_str(),m_curIndex);
		}
		std::string fileStr = fileName;
		Poco::FileOutputStream fos(fileStr, std::ios::out|std::ios::app);
		fos << cmmAlarmStr.c_str();
		fos.close();
		return 0;
	}

	void CMMLogFile::GenDirName( CData &dir )
	{
		Poco::DateTime now;
		char dirStr[32] = {0};
		sprintf(dirStr, "/Alarm/%04d%02d%02d", now.year(), now.month(), now.day());
		dir = dirStr;
	}

	void CMMLogFile::FormatAlarmInfo(TAlarm &alarm, CData &info )
	{		
		info =  alarm.SerialNo+"|@"
				+alarm.ID+"|@"
				+alarm.DeviceID+"|@"
				+alarm.NMAlarmID+"|@"
				+alarm.AlarmTime+"|@"
				+CData(alarm.AlarmLevel)+"|@"
				+alarm.AlarmFlag+"|@"
				+alarm.AlarmDesc+"|@"
				+alarm.AlarmRemark2+"|@"
				+CData(alarm.SignalNumber)+"|@"
				+alarm.AlarmRemark1+"\n\r";
	}

}



