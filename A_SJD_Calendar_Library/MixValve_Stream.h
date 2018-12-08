#pragma once

#include "A__Constants.h"
#include "D_DataStream.h"
#include "D_EpData.h"

class MixValve_Run;
class MixValve_EpMan;

class MixValve_Stream : public DataStreamT<EpDataT<MIXV_DEF>, MixValve_Run, MixValve_EpMan, MV_COUNT> {
public:
	MixValve_Stream(MixValve_Run & run);
private:
	char * objectFldStr(S1_ind activeInd, bool inEdit) const;
};