#pragma once

#include "Zone_Stream.h"
#include "D_EpData.h"

class TowelR_Run;
class Zone_Run;

class TowelR_Stream : public Zone_Stream
{
public:	
	TowelR_Stream(Zone_Run & run);
private:	
	const char * getSetupPrompts(S1_ind item) const {return setupPrompts[item];}
	static const char setupPrompts[TR_COUNT][13];
	ValRange addValToFieldStr(S1_ind item, U1_byte myVal = 0) const;
	S1_newPage onNameSelect() {return DataStream::onNameSelect();}
	char * objectFldStr(S1_ind activeInd, bool inEdit) const;
};
