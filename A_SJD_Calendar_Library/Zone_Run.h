#pragma once

#include "debgSwitch.h"
#include "D_Operations.h"
#include "D_EpData.h"
#include "DateTime_A.h"
#include "Zone_Stream.h"
#include "GetExpCurveConsts.h"

class FactoryObjects;

struct TTvalStruct;


class Zone_Run : public RunT<Zone_EpD>
{
public:
	Zone_Run();
	// Queries
	bool isDHWzone() const;
	bool isHeating() const;
	S1_byte getFlowSensTemp() const;
	U1_byte getCurrProgTempRequest() const {return _currProgTempRequest;}
	U1_byte getCurrTempRequest() const {return _currTempRequest;}
	//U1_byte getNthDP(S1_byte position) const;
	//U1_byte getCurrDP() const {return currentDP;}
	S2_byte getFractionalCallSensTemp() const;
	U1_ind modifiedCallTemp(U1_byte callTemp) const;
	bool ttExpired();
	TTvalStruct timeTempForDP(U1_ind dp, DTtype checkTime) const;
	S1_byte findDT(DTtype originalFromDate, DTtype originalToDate) const;
	// modifiers
	U1_byte getNthDT(ZdtData & zd, S1_byte position);
	bool check(); // setZFlowTemp & switches relays 
#if defined (TEMP_SIMULATION)
	void setcurrTempRequest(S1_byte myTemp);
#endif
	void loadDTSequence(); // recycles old DT's
	//void setCurrDP(U1_ind newDP) {currentDP = newDP;}
	// Diary Methods
	void refreshCurrentTT(FactoryObjects & fact);

	static bool checkProgram(bool force = false);

	volatile static long z_period;
	U1_byte getCallFlowT() const {return _callFlowTemp;} // only for simulator & reporting
	#if defined (ZPSIM)
	S1_byte tempRatio() {return _tempRatio;} // required by simulator
	#endif
private:
	enum {SPARE_DT = 127};
	struct startTime {
		DTtype	start;
		S1_ind  dt;
		bool operator < (startTime rhs) {return start < rhs.start;}
	};
	struct nestedSpecials {
		nestedSpecials();
		S1_ind addDT(S1_ind dt, S1_ind & prevWkly); // returns removed DT index for possible recycling
		S1_ind dtArr[5];
		U1_ind end;
	};	
	bool setZFlowTemp(); // sets flow temps. Returns true if needs heat
	// Diary Methods
	S1_ind getLastDP(E_dpTypes dpType) const;
	U1_ind currentWeeklyDT(ZdtData & zd, DTtype thisDate) const;
	U1_ind findDPforDOW(S2_byte currDP, S2_byte dayNo) const;
	U1_ind findTTforTime(U1_ind currDP, DTtype myTime, S2_byte & nextDayNo, S2_byte & nextTT) const;
	void sortSequence(U1_byte count);
	void populateSequence(U1_ind firstDT, U1_byte dtCount, U1_byte arrSize);
	void removeNestedDTs(U1_byte arrSize);
	void removePreNowDTs(U1_byte arrSize);
	void recycleDT(S1_ind prevWkly, S1_ind t, nestedSpecials & nestSpec);

	S1_byte _currProgTempRequest;
	S1_byte _currTempRequest;
	S1_byte _nextTemp;
	DTtype _ttEndDateTime;
	volatile U1_byte _callFlowTemp;		// Programmed flow temp, modified by zoffset
	volatile U1_byte _tempRatio;
	long _aveError; // in 1/16 degrees
	long _aveFlow;  // in degrees
	startTime * _sequence;
	bool _isHeating; // just for logging
	GetExpCurveConsts _getExpCurve;
};


