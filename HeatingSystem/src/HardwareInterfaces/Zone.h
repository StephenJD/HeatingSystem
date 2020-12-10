#pragma once
#include <RDB.h>
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino
#include "Date_Time.h"
#include <CurveMatching.h>
#include "..\Assembly\HeatingSystemEnums.h"
#include "..\Client_DataStructures\Data_Zone.h"

//***************************************************
//              Zone Dynamic Class
//***************************************************
//namespace client_data_structures {
//	struct R_Zone;
//}

namespace Assembly {	
	class TemperatureController;
	class Sequencer;
}

namespace HardwareInterfaces {
	class ThermalStore;
	class UI_Bitwise_Relay;
	class UI_TempSensor;
	class MixValveController;

	class Zone : public LCD_UI::VolatileData {
	public:
		Zone() = default;
		void initialise(
			int zoneID
			, UI_TempSensor & callTS
			, UI_Bitwise_Relay & callRelay
			, ThermalStore & thermalStore
			, MixValveController & mixValveController
			, int8_t maxFlowTemp
			, Assembly::Sequencer& sequencer
		);
#ifdef ZPSIM
		Zone(UI_TempSensor & ts, int reqTemp, UI_Bitwise_Relay & callRelay);
#endif
		// Queries
		RelationalDatabase::RecordID id() const { return _recordID; }
		int8_t getCallFlowT() const { return _callFlowTemp; } // only for simulator & reporting
		int8_t currTempRequest() const { return modifiedCallTemp(_currProfileTempRequest); }
		int8_t nextTempRequest() const { return modifiedCallTemp(_nextProfileTempRequest); }
		int8_t maxUserRequestTemp() const;
		int8_t offset() const { return _offsetT; }
		int8_t getCurrTemp() const;
		bool isCallingHeat() const;
		Date_Time::DateTime nextEventTime() { return _ttEndDateTime; }
		bool operator== (const Zone & rhs) { return _recordID == rhs._recordID; }
		bool isDHWzone() const;
		int16_t getFractionalCallSensTemp() const;

		// Modifier
		void refreshProfile();
		void resetOffsetToZero() { _offsetT = 0; }
		void offsetCurrTempRequest(int8_t val);
		bool setFlowTemp();
		void setProfileTempRequest(int8_t temp) { _currProfileTempRequest = temp; }
		void setNextProfileTempRequest(int8_t temp) { _nextProfileTempRequest = temp; }
		void setNextEventTime(Date_Time::DateTime time) { _ttEndDateTime = time; }
		void preHeatForNextTT();
		auto zoneRecord() -> RelationalDatabase::Answer_R<client_data_structures::R_Zone> &;

	private:
		int8_t modifiedCallTemp(int8_t callTemp) const;
		Assembly::Sequencer * _sequencer = 0;
		UI_TempSensor * _callTS = 0;
		UI_Bitwise_Relay * _relay = 0;
		ThermalStore * _thermalStore = 0;
		MixValveController * _mixValveController;
		RelationalDatabase::Answer_R<client_data_structures::R_Zone> _zoneRecord;
		RelationalDatabase::RecordID _recordID = 0;
		int8_t _offsetT = 0;
		uint8_t _maxFlowTemp = 0;

		int8_t _currProfileTempRequest = 0;
		int8_t _nextProfileTempRequest = 0;
		Date_Time::DateTime _ttEndDateTime;
		int8_t _callFlowTemp = 0;		// Programmed flow temp, modified by zoffset
		bool _isHeating = false; // just for logging
		GP_LIB::GetExpCurveConsts _getExpCurve{ 128 };
	};
}
