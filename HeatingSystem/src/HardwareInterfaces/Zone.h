#pragma once
#include <RDB.h>
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino
#include "Date_Time.h"

//***************************************************
//              Zone Dynamic Class
//***************************************************

namespace HardwareInterfaces {
	class ThermalStore;
	class Bitwise_Relay;
	class I2C_Temp_Sensor;
	class MixValveController;

	class Zone : public LCD_UI::VolatileData {
	public:
		Zone() = default;
#ifdef ZPSIM
		Zone(I2C_Temp_Sensor & ts, int reqTemp, Bitwise_Relay & callRelay);
#endif
		// Queries
		RelationalDatabase::RecordID record() const { return _recordID; }
		int8_t getCallFlowT() const { return _callFlowTemp; } // only for simulator & reporting
		//int8_t getFlowSensTemp() const;
		int8_t currTempRequest() const { return modifiedCallTemp(_currProfileTempRequest); }
		int8_t nextTempRequest() const { return modifiedCallTemp(_nextProfileTempRequest); }
		int8_t maxUserRequestTemp() const;
		int8_t offset() const { return _offsetT; }
		int8_t getCurrTemp() const;
		bool isCallingHeat() const; // { return getCurrTemp() < modifiedCallTemp(_currProfileTempRequest); }
		Date_Time::DateTime nextEventTime() { return _ttEndDateTime; }
		bool operator== (const Zone & rhs) { return _recordID == rhs._recordID; }
		bool isDHWzone() const;
		int16_t getFractionalCallSensTemp() const;

		// Modifier
		void initialise(int zoneID, I2C_Temp_Sensor & callTS, Bitwise_Relay & callRelay, ThermalStore & thermalStore, MixValveController & mixValveController, int8_t maxFlowTemp);
		void offsetCurrTempRequest(int8_t val);
		bool setFlowTemp();
		void setProfileTempRequest(int8_t temp) { _currProfileTempRequest = temp; }
		void setNextProfileTempRequest(int8_t temp) { _nextProfileTempRequest = temp; }
		void setNextEventTime(Date_Time::DateTime time) { _ttEndDateTime = time; }

	private:
		int8_t modifiedCallTemp(int8_t callTemp) const;

		I2C_Temp_Sensor * _callTS = 0;
		Bitwise_Relay * _relay = 0;
		ThermalStore * _thermalStore = 0;
		MixValveController * _mixValveController;
		RelationalDatabase::RecordID _recordID = 0;
		int8_t _offsetT = 0;
		int8_t _maxFlowTemp = 0;

		int8_t _currProfileTempRequest = 0;
		int8_t _nextProfileTempRequest = 0;
		Date_Time::DateTime _ttEndDateTime;
		int8_t _callFlowTemp = 0;		// Programmed flow temp, modified by zoffset
		//uint8_t _tempRatio = 0;
		//long _aveError = 0; // in 1/16 degrees
		//long _aveFlow = 0;  // in degrees
		//startTime * _sequence;
		bool _isHeating = false; // just for logging
		//GetExpCurveConsts _getExpCurve;
	};

	//extern Zone * zones; // Array of Zones provided by client

}
