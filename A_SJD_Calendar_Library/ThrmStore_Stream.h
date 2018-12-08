#pragma once
#include "D_DataStream.h"
#include "D_EpData.h"
#include "A__Constants.h"

class ThrmSt_Run;
class ThrmStore_EpMan;

class ThrmStore_Stream : public DataStreamT<EpDataT<THRMST_DEF>, ThrmSt_Run, ThrmStore_EpMan, TMS_COUNT>
{
public:
	enum ThrmStoreInfo {i_MainsT,i_TopT,i_MidT,i_LwrT,i_PreT,i_dhwT,i_Cond,i_StoveT,i_CoilT,i_GasT,i_NoOfInfo};
	ThrmStore_Stream(ThrmSt_Run & run);
	const U1_byte getNoOfStreamItems() const; // returns no of members of the Collection -1
private:
	char * objectFldStr(S1_ind activeInd, bool inEdit) const;
};

