#pragma once
#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <Mix_Valve.h>
#include <RDB.h>
#include "A__Constants.h"
#if defined (ZPSIM)
#include <ostream>
#endif

namespace HardwareInterfaces {
	class UI_Bitwise_Relay;
	class UI_TempSensor;

	class MixValveController : public I_I2Cdevice_Recovery {
	public:
		using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
		MixValveController() = default;

		// Queries
		uint8_t flowTemp() const;
		bool zoneHasControl(uint8_t zoneRelayID) const { return _controlZoneRelay == zoneRelayID; }
		// Virtual Functions
		I2C_Talk_ErrorCodes::error_codes testDevice() override;
		//uint8_t initialiseDevice() override;

		bool needHeat(bool isHeating); // used by ThermStore.needHeat	
		uint8_t readFromValve(Mix_Valve::Registers reg); // returns value
		uint8_t sendSetup();
		void setResetTimePtr(unsigned long * timeOfReset_mS) { _timeOfReset_mS = timeOfReset_mS; }
		bool amControlZone(uint8_t callTemp, uint8_t maxTemp, uint8_t zoneRelayID);
		bool check();

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
		void initialise(int index, int addr, UI_Bitwise_Relay * relayArr, UI_TempSensor * tempSensorArr, int flowTempSens, int storeTempSens);
	private:
		bool controlZoneRelayIsOn() const;
		I2C_Talk_ErrorCodes::error_codes writeToValve(Mix_Valve::Registers reg, uint8_t value); // returns I2C Error 
		//void setLimitZone(int mixValveIndex);

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