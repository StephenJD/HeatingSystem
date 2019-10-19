#pragma once
#include <RDB.h>
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino
#include "Date_Time.h"

//***************************************************
//              Zone Dynamic Class
//***************************************************

namespace HardwareInterfaces {
	class ThermalStore;
	class Relay;
	class I2C_Temp_Sensor;
	class MixValveController;

	class Zone : public LCD_UI::VolatileData {
	public:
		Zone() = default;
#ifdef ZPSIM
		Zone(I2C_Temp_Sensor & ts, int reqTemp);
#endif
		// Queries
		RelationalDatabase::RecordID record() const { return _recID; }
		uint8_t getCallFlowT() const { return _callFlowTemp; } // only for simulator & reporting
		//int8_t getFlowSensTemp() const;
		uint8_t currTempRequest() const { return modifiedCallTemp(_currProfileTempRequest); }
		uint8_t nextTempRequest() const { return modifiedCallTemp(_nextProfileTempRequest); }
		uint8_t offset() const { return _offsetT; }
		uint8_t getCurrTemp() const;
		bool isCallingHeat() const { return getCurrTemp() < modifiedCallTemp(_currProfileTempRequest); }
		Date_Time::DateTime nextEventTime() { return _ttEndDateTime; }
		bool operator== (const Zone & rhs) { return _recID == rhs._recID; }
		bool isDHWzone() const;
		int16_t getFractionalCallSensTemp() const;

		// Modifier
		void initialise(int zoneID, I2C_Temp_Sensor & callTS, Relay & callRelay, ThermalStore & thermalStore, MixValveController & mixValveController, int8_t maxFlowTemp);
		void offsetCurrTempRequest(uint8_t val);
		bool setFlowTemp();
		void setProfileTempRequest(uint8_t temp) { _currProfileTempRequest = temp; }
		void setNextProfileTempRequest(uint8_t temp) { _nextProfileTempRequest = temp; }
		void setNextEventTime(Date_Time::DateTime time) { _ttEndDateTime = time; }

	private:
		uint8_t modifiedCallTemp(uint8_t callTemp) const;

		I2C_Temp_Sensor * _callTS = 0;
		Relay * _relay = 0;
		ThermalStore * _thermalStore = 0;
		MixValveController * _mixValveController;
		RelationalDatabase::RecordID _recID = 0;
		int8_t _offsetT = 0;
		int8_t _maxFlowTemp = 0;

		int8_t _currProfileTempRequest = 0;
		int8_t _nextProfileTempRequest = 0;
		Date_Time::DateTime _ttEndDateTime;
		uint8_t _callFlowTemp = 0;		// Programmed flow temp, modified by zoffset
		//uint8_t _tempRatio = 0;
		//long _aveError = 0; // in 1/16 degrees
		//long _aveFlow = 0;  // in degrees
		//startTime * _sequence;
		bool _isHeating = false; // just for logging
		//GetExpCurveConsts _getExpCurve;
	};

	//extern Zone * zones; // Array of Zones provided by client

}
