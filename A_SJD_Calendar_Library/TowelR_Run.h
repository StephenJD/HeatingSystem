#pragma once
#include "D_EpData.h"

class TowelR_Run : public RunT<EpDataT<TWLRL_DEF> >
{
public:
	TowelR_Run();
	bool check(); // returns true if ON
private:
	friend class TowelR_Stream;
	S1_byte getCallSensTemp() const;
	bool rapidTempRise() const;
	U1_byte sharedZoneCallTemp(U1_ind callRelay) const;
	bool setFlowTemp();

	U1_byte _callFlowTemp;
	mutable U2_byte _timer;
	mutable U1_byte _prevTemp;

};

