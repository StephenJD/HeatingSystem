#pragma once
#include "debgSwitch.h"

#include "D_Operations.h"
#include "D_EpData.h"

class TempSens_Run : public RunT<EpDataT<TEMPS_DEF> >
{
public:
	TempSens_Run();
	S1_byte getSensTemp() const;
	S2_byte getFractionalSensTemp() const;

public:
	mutable S2_byte lastGood;
	mutable unsigned long timeExpired;
	#if defined (ZPSIM)
		void setTemp(S1_byte temp);
		void changeTemp(S1_byte change);
		static uint8_t tempSens[27];
	#endif

};
