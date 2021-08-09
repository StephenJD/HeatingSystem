#pragma once
#include <RDB.h>
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino
#include <Date_Time.h>
#include "..\Assembly\HeatingSystemEnums.h"
#include "..\Client_DataStructures\Rec_Zone.h"
#include <math.h>

//***************************************************
//              Zone Dynamic Class
//***************************************************

namespace Assembly {	
	class TemperatureController;
	class Sequencer;
	struct ProfileInfo;
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
			RelationalDatabase::Answer_R<client_data_structures::R_Zone> zoneRecord
			, UI_TempSensor& callTS
			, UI_Bitwise_Relay& callRelay
			, ThermalStore& thermalStore
			, MixValveController& mixValveController
		);
#ifdef ZPSIM
		Zone(UI_TempSensor& ts, int reqTemp, UI_Bitwise_Relay& callRelay);
#endif
		static void setSequencer(Assembly::Sequencer& sequencer) { _sequencer = &sequencer; }
		// Queries
		RelationalDatabase::RecordID id() const { return _recordID; }
		int8_t getCallFlowT() const { return _callFlowTemp; } // only for simulator & reporting
		int8_t currTempRequest() const { return modifiedCallTemp(_currProfileTempRequest); }
		int8_t preheatTempRequest() const { return _preheatCallTemp; }
		int8_t nextTempRequest() const { return modifiedCallTemp(_nextProfileTempRequest); }
		int8_t maxUserRequestTemp(bool setLimit) const;
		int8_t getCurrTemp() const;
		bool isCallingHeat() const;
		Date_Time::DateTime nextEventTime() const { return _ttEndDateTime; }
		bool operator== (const Zone& rhs) const { return _recordID == rhs._recordID; }
		bool isDHWzone() const;
		int16_t getFractionalCallSensTemp() const;
		const RelationalDatabase::Answer_R<client_data_structures::R_Zone>& zoneRecord() const { return _zoneRecord; }
		Date_Time::DateTime startDateTime() { return _ttStartDateTime; }
		// Modifier
		Assembly::ProfileInfo refreshProfile(bool reset = true);
		void cancelPreheat();
		void changeCurrTempRequest(int8_t val);
		bool setFlowTemp();
		void setProfileTempRequest(int8_t temp) { _currProfileTempRequest = temp; }
		void setNextProfileTempRequest(int8_t temp) { _nextProfileTempRequest = temp; }
		void setNextEventTime(Date_Time::DateTime time) { _ttEndDateTime = time; }
		void setTTStartTime(Date_Time::DateTime time) { _ttStartDateTime = time;}
		void preHeatForNextTT();
		RelationalDatabase::Answer_R<client_data_structures::R_Zone>& zoneRecord() { return _zoneRecord; }
		static constexpr double ERROR_DIVIDER = 2.;
		static constexpr double RATIO_DIVIDER = 255.;
		static constexpr double MAX_RATIO = (254000 / 255) / 1000.;
		static constexpr int REQ_ACCUMULATION_PERIOD = 120; // minutes
		static constexpr uint8_t DELAY_COOLING_TIME = 120;
		static constexpr uint8_t MEASURING_DELAY = 255;
		static constexpr int16_t PREHEAT_ENDED = -1;
		static double uncompressTC(uint8_t compressed_tc) {
			return exp(compressed_tc / 50.) * 20;
		}

		static uint8_t compressTC(double timeConst) {
			if (timeConst > 3280) timeConst = 3280;
			if (timeConst < 20) timeConst = 20;
			return static_cast<uint8_t>(::log(timeConst / 20.) * 50. + 0.5);
		}
	private:
		int8_t modifiedCallTemp(int8_t callTemp) const;
		void saveThermalRatio();

		UI_TempSensor* _callTS = 0;
		UI_Bitwise_Relay* _relay = 0;
		ThermalStore* _thermalStore = 0;
		MixValveController* _mixValveController;

		RelationalDatabase::Answer_R<client_data_structures::R_Zone> _zoneRecord;
		RelationalDatabase::RecordID _recordID = 0;
		uint8_t _maxFlowTemp = 0;
		uint16_t _startCallTemp = 0;
		int16_t _minsInPreHeat = PREHEAT_ENDED;
		/// <summary>
		/// Controls saving of new delay period.
		/// Cooling State: Is incremented up to DELAY_COOLING_TIME whilst zone is cooling.
		/// Ready State: == DELAY_COOLING_TIME(120) - Can re-measure delay.
		/// Measuring Delay State: == MEASURING_DELAY(255)
		/// </summary>
		uint8_t _minsCooling = 0;
		mutable uint8_t _averagePeriod = 0;

		int8_t _currProfileTempRequest = 0; // current profile temp. - user can advance to next.
		int8_t _nextProfileTempRequest = 0; // next selectable profile temp.
		int8_t _preheatCallTemp = 0; // current called-for temp, adjusted for pre-heat.
		Date_Time::DateTime _ttStartDateTime;
		Date_Time::DateTime _ttEndDateTime;
		mutable uint16_t _rollingAccumulatedRatio = 0;
		uint16_t _timeConst;
		int8_t _callFlowTemp = 0;
		bool _finishedFastHeating = false;
		static Assembly::Sequencer* _sequencer;
	};
}
