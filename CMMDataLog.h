#ifndef _CMMDATALOG_H
#define  _CMMDATALOG_H
#include "Poco/DateTime.h"
#include "Data.h"

#include <map>
#include <string>
#include <list>
#include <vector>

namespace CMM_SJY
{
	class CMMDataLog
	{
	public:
		CMMDataLog();
		~CMMDataLog();
		void Save(std::map<CData, CData>& msg);
	private:
		void GenFileName(Poco::DateTime &date, CData &fileName);

	private:
		CData m_fileName;//canyon
		int m_index;
	};
}
#endif
