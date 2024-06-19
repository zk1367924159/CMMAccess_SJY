#ifndef _CMMMETETRANSLATE_H
#define _CMMMETETRANSLATE_H
#include "map"
#include "Data.h"


namespace CMM_SJY
{
	class CMeteTranslate
	{
	private:
		std::map<CData, CData> m_metesMap;
		static CMeteTranslate* _instance;
	public:
		static CMeteTranslate* Instance();
		void Init();
		CData FromInnerToCMM(int id, int &SignalNumber);
		CData FromInnerToCMM(CData id);
		CData FromCMMToInner( CData cmmId, int signalNumber );
		int FromInnerPortTypeToCMM(int type, int dataType);

		int ConvertToCmmMeterType(CData meterType);
	};
}
#endif
