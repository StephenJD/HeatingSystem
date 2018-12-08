#pragma once

#include "D_Operations.h"
#include "D_EpData.h"

class ThrmSt_Run : public RunT<EpDataT<THRMST_DEF> >
{
public:
	ThrmSt_Run();
	S2_byte getFractionalOutsideTemp() const;
	U1_byte getTopTemp() const;
	U1_byte getOutsideTemp() const;
	//bool storeIsUpToTemp() const; // just returns status flag
	bool dumpHeat() const;
	U1_byte currDeliverTemp() const {return _currDeliverTemp;}
	U1_byte calcCurrDeliverTemp(U1_byte callTemp) const;

	bool check(); // checks zones, twl rads & thm store for heat requirement and sets relays accordingly
	void setLowestCWtemp(bool isFlowing);
	void adjustHeatRate(byte change);
	bool needHeat(U1_byte currRequest, U1_byte nextRequest);
	U1_byte getGroundTemp() const;
	void calcCapacities();

private:
	bool dhwDeliveryOK(U1_byte currRequest) const;
	bool dhwNeedsHeat(U1_byte callTemp, U1_byte nextRequest);

	U1_byte _currDeliverTemp;
	bool _isHeating;

	float _upperC, _midC, _bottomC; // calculated capacity factors
	float _upperV, _midV, _bottomV; // calculated Volumes
public:
	U1_byte _groundT;
};

