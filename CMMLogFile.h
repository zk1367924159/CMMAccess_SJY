#ifndef _CMMLOGFILE_H
#define _CMMLOGFILE_H
#include "Data.h"
#include "CMMCommonStruct.h"
namespace CMM_SJY
{
	class CMMLogFile
	{
	private:		
		int m_perFileSize;
		int m_curIndex;
		CData m_curDir;
	public:
		CMMLogFile();
		int SetFileSize(int fileSize);
		int log(TAlarm &alarm);
	private:
		void GenDirName(CData &dir);
		void FormatAlarmInfo(TAlarm &alarm, CData &info);

	};
}
#endif

