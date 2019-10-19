#pragma once
#include "Arduino.h"
#include <I2C_Talk.h>
#include <Mix_Valve.h>
#include "Temp_Sensor.h"
#include <RDB.h>
#include "Relay.h"
#include "A__Constants.h"
#if defined (ZPSIM)
#include <ostream>
#endif

namespace HardwareInterfaces {

	class MixValveController : public I_I2Cdevice_Recovery {
	public:
		using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
		MixValveController() = default;

		// Virtual Functions
		I2C_Talk_ErrorCodes::error_codes testDevice() override;
		//uint8_t initialiseDevice() override;

		bool needHeat(bool isHeating); // used by ThermStore.needHeat	
		uint8_t readFromValve(Mix_Valve::Registers reg); // returns value
		uint8_t sendSetup();
		void setResetTimePtr(unsigned long * timeOfReset_mS) { _timeOfReset_mS = timeOfReset_mS; }
		bool amControlZone(uint8_t callTemp, uint8_t maxTemp, uint8_t relayID);
		bool check();

//#if defined (ZPSIM)
//		int16_t getValvePos() const; // public for simulator
//		void reportMixStatus() const;
//		int8_t controlRelay() const { return _controlZoneRelay; }
//		uint8_t mixCallT() const { return _mixCallTemp; }
//		int8_t getState() const; // 1 = heat, 0 = off, -1 = cool
//
//		volatile int8_t motorState = 0; // required by simulator
//		static std::ofstream lf;
//#endif
		void initialise(int index, int addr, Relay * relayArr, I2C_Temp_Sensor * tempSensorArr, int flowTempSens, int storeTempSens);
	private:
		bool controlZoneRelayIsOn() const;
		I2C_Talk_ErrorCodes::error_codes writeToValve(Mix_Valve::Registers reg, uint8_t value); // returns I2C Error 
		uint8_t getStoreTemp() const;
		//void setLimitZone(int mixValveIndex);

		I2C_Temp_Sensor * _tempSensorArr = 0;
		Relay * _relayArr = 0;
		unsigned long * _timeOfReset_mS = 0;
		uint8_t _error = 0;
		uint8_t _index = 0;
		RelationalDatabase::RecordID _flowTempSens;
		RelationalDatabase::RecordID _storeTempSens;
		volatile uint8_t _mixCallTemp = MIN_FLOW_TEMP;
		volatile uint8_t _controlZoneRelay = 0;
		uint8_t _limitTemp = 60;
	};
}