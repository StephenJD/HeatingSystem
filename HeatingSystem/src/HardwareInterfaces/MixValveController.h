#pragma once
#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <Mix_Valve.h>
#include <RDB.h>
#include "A__Constants.h"
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino

#if defined (ZPSIM)
#include <ostream>
#endif

namespace HardwareInterfaces {
	class UI_Bitwise_Relay;
	class UI_TempSensor;

	class MixValveController : public I_I2Cdevice_Recovery, public LCD_UI::VolatileData {
	public:
		using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
		MixValveController() = default;
		void initialise(int index, int addr, UI_Bitwise_Relay * relayArr, UI_TempSensor * tempSensorArr, int flowTempSens, int storeTempSens);

		// Virtual Functions
		I2C_Talk_ErrorCodes::Error_codes testDevice() override;

		// Queries
		uint8_t flowTemp() const;
		bool zoneHasControl(uint8_t zoneRelayID) const { return _controlZoneRelay == zoneRelayID; }
		int8_t relayInControl() const;
		uint8_t index() const { return _index; }

		// Modifiers
		bool needHeat(bool isHeating); // used by ThermStore.needHeat	
		int16_t readFromValve(Mix_Valve::Registers reg); // returns value
		uint8_t sendSetup();
		void setResetTimePtr(unsigned long * timeOfReset_mS) { _timeOfReset_mS = timeOfReset_mS; }
		bool amControlZone(uint8_t callTemp, uint8_t maxTemp, uint8_t zoneRelayID);
		bool check();
		void setRequestFlowTemp(uint8_t callTemp);

//#if defined (ZPSIM)
//		int16_t getValvePos() const; // public for simulator
//		void reportMixStatus() const;
//		int8_t controlRelay() const { return _controlZoneRelay; }
//		uint8_t mixCallT() const { return _mixCallTemp; }
//		int8_t logicalState() const; // 1 = heat, 0 = off, -1 = cool
//
//		volatile int8_t motorState = 0; // required by simulator
//		static std::ofstream lf;
//#endif
	private:
		I2C_Talk_ErrorCodes::Error_codes writeToValve(Mix_Valve::Registers reg, uint8_t value); // returns I2C Error 

		UI_TempSensor * _tempSensorArr = 0;
		UI_Bitwise_Relay * _relayArr = 0;
		unsigned long * _timeOfReset_mS = 0;
		uint8_t _error = 0;
		uint8_t _index = 0;
		RelationalDatabase::RecordID _flowTempSens;
		RelationalDatabase::RecordID _storeTempSens;
		volatile uint8_t _mixCallTemp = MIN_FLOW_TEMP;
		volatile uint8_t _controlZoneRelay = 0;
		uint8_t _limitTemp = 100;
	};
}