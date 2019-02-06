#pragma once
#include "Arduino.h"
#include <I2C_Helper.h>
#include <Mix_Valve/Mix_Valve.h>
#include "Temp_Sensor.h"
#include <RDB/src/RDB.h>
#include "Relay.h"
#include "A__Constants.h"
#if defined (ZPSIM)
#include <ostream>
#endif

namespace HardwareInterfaces {

	class MixValveController : public I2C_Helper::I_I2Cdevice {
	public:
		void setResetTimePtr(unsigned long * timeOfReset_mS) { 
			_timeOfReset_mS = timeOfReset_mS;
		}
		// Queries

		// Virtual Functions
		uint8_t testDevice(I2C_Helper & i2c, int addr) override;
		//uint8_t initialiseDevice() override;

		bool needHeat(bool isHeating) const; // used by ThermStore.needHeat	
		uint8_t sendSetup() const;
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
		uint8_t readFromValve(Mix_Valve::Registers reg) const;
	private:
		I2C_Temp_Sensor * _tempSensorArr = 0;
		Relay * _relayArr = 0;
		uint8_t _index = 0;
		RelationalDatabase::RecordID _flowTempSens;
		RelationalDatabase::RecordID _storeTempSens;

		bool controlZoneRelayIsOn() const;
		uint8_t writeToValve(Mix_Valve::Registers reg, uint8_t value) const; // returns I2C Error 
		uint8_t getStoreTemp() const;

		//void setLimitZone(int mixValveIndex);

		volatile uint8_t _mixCallTemp = MIN_FLOW_TEMP;
		volatile uint8_t _controlZoneRelay = 0;
		uint8_t _limitTemp = 60;
		int _error = 0;
		unsigned long * _timeOfReset_mS = 0;
	};
	//extern MixValveController * mixValveController;

}