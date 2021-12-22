#pragma once
#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <I2C_Registers.h>
#include <Mix_Valve.h>
#include <RDB.h>
#include "A__Constants.h"
#include "I2C_Comms.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino

#if defined (ZPSIM)
#include <ostream>
#endif

namespace HardwareInterfaces {
	class UI_Bitwise_Relay;
	//class UI_TempSensor;

	class MixValveController : public I_I2Cdevice_Recovery, public LCD_UI::VolatileData {
	public:
		MixValveController(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers);
		void initialise(int index, int addr, UI_Bitwise_Relay * relayArr, int flowTS_addr, UI_TempSensor & storeTempSens, unsigned long& timeOfReset_mS, bool multi_master_mode);

		// Virtual Functions
		I2C_Talk_ErrorCodes::Error_codes testDevice() override;

		// Queries
		uint8_t flowTemp() const;
		uint8_t reqTemp() const {return _mixCallTemp;}
		bool zoneHasControl(uint8_t zoneRelayID) const { return _controlZoneRelay == zoneRelayID; }
		int8_t relayInControl() const;
		uint8_t index() const { return _regOffset == MV_REG_MASTER_0_OFFSET ? 0 : 1; }
		const __FlashStringHelper* showState() const;
		uint8_t getReg(int reg) const;
		bool multi_master_mode() const { return _is_multimaster; }

		// Modifiers
		bool needHeat(bool isHeating); // used by ThermStore.needHeat	
		void readRegistersFromValve(); // returns value
		uint8_t sendSlaveIniData(uint8_t requestINI_flag);
		bool amControlZone(uint8_t callTemp, uint8_t maxTemp, uint8_t zoneRelayID);
		bool check();
		void sendRequestFlowTemp(uint8_t callTemp);
		void logMixValveOperation(bool logThis);
		void monitorMode();
		void enable_multi_master_mode(bool enable);

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
		I2C_Talk_ErrorCodes::Error_codes writeToValve(int reg, uint8_t value);
		void waitForWarmUp();
		void setReg(int reg, uint8_t value);


		UI_TempSensor * _storeTempSens = 0;
		UI_Bitwise_Relay * _relayArr = 0;
		unsigned long * _timeOfReset_mS = 0;
		i2c_registers::I_Registers& _prog_registers;

		UI_TempSensor _slaveMode_flowTempSensor;

		Mix_Valve::Mode _previous_valveStatus[NO_OF_MIXERS];
		bool _is_multimaster = false;
		uint8_t _error = 0;
		uint8_t _regOffset = 0;
		uint8_t _limitTemp = 100;
		uint8_t _flowTS_addr = 0;
		volatile uint8_t _mixCallTemp = MIN_FLOW_TEMP;
		volatile uint8_t _controlZoneRelay = 0;
	};
}