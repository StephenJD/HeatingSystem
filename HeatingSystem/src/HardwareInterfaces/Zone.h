#pragma once
#include <RDB/src/RDB.h>
#include <HeatingSystem/src/LCD_UI/I_Record_Interface.h>
#include "Date_Time.h"

//***************************************************
//              Zone Dynamic Class
//***************************************************

namespace HardwareInterfaces {

	class Zone : public LCD_UI::VolatileData {
	public:
		Zone(int tempReq, int currTemp);
		// Queries
		RelationalDatabase::RecordID record() const { return _recID; }
		uint8_t getCallFlowT() const { return _callFlowTemp; } // only for simulator & reporting
		//int8_t getFlowSensTemp() const;
		uint8_t getCurrProgTempRequest() const { return _currProgTempRequest; }
		uint8_t getCurrTempRequest() const { return _currTempRequest; }
		uint8_t getCurrTemp() const { return _currTemp; }
		bool isCallingHeat() const { return _currTemp < _currTempRequest; }
		bool operator== (const Zone & rhs) { return _recID == rhs._recID; }

		// Modifier
		void initialise(RelationalDatabase::RecordID zoneID, int tempReq, int currTemp);
		void setCurrTempRequest(uint8_t val) { _currTempRequest = val; }
		void setCurrTemp(uint8_t val) { _currTemp = val; }
	private:
		RelationalDatabase::RecordID _recID = 0;
		int8_t _callTempSensIndex = 0;
		int8_t _relayIndex = 0;
		int8_t _mixValveIndex = 0;
		int8_t _currProgTempRequest = 0;
		int8_t _currTempRequest = 0;
		int8_t _currTemp = 0;
		int8_t _nextTemp = 0;
		Date_Time::DateTime _ttEndDateTime;
		uint8_t _callFlowTemp = 0;		// Programmed flow temp, modified by zoffset
		uint8_t _tempRatio = 0;
		long _aveError = 0; // in 1/16 degrees
		long _aveFlow = 0;  // in degrees
		//startTime * _sequence;
		bool _isHeating = false; // just for logging
		//GetExpCurveConsts _getExpCurve;
	};

	//extern Zone * zones; // Array of Zones provided by client

}
