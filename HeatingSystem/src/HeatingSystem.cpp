#include "HeatingSystem.h"
#include "Assembly\FactoryDefaults.h"
#include "HardwareInterfaces\I2C_Comms.h"
#include "HardwareInterfaces\A__Constants.h"
#include "LCD_UI\A_Top_UI.h"
#include <EEPROM.h>
#include <Clock.h>
#include <MemoryFree.h>

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif

using namespace client_data_structures;
using namespace RelationalDatabase;
using namespace HardwareInterfaces;
using namespace arduino_logger;

//extern Remote_display * hall_display;
//extern Remote_display * flat_display;
//extern Remote_display * us_display;
void ui_yield();

namespace HeatingSystemSupport {
	// Due has 96kB RAM
	uint8_t virtualProm[RDB_START_ADDR + RDB_MAX_SIZE];

	void initialise_virtualROM() {
#if defined(__SAM3X8E__)		
		logger() << L_time << F("  virtualROM...\n");
		eeprom().readEP(RDB_START_ADDR, RDB_MAX_SIZE, virtualProm + RDB_START_ADDR);
		logger() << F("  virtualROM Complete. Size: ") << sizeof(virtualProm) << L_endl;
#endif
	}

	int writer(int address, const void * data, int noOfBytes) {
		//if (address == 495) {
		//	auto debug = false;
		//}
		uint8_t status = 0;
		if (address < RDB_START_ADDR || address + noOfBytes > RDB_START_ADDR + RDB_MAX_SIZE) {
			logger() << F("Illegal RDB write address: ") << int(address) << L_endl;
			return -1;
		}
		const unsigned char * byteData = static_cast<const unsigned char *>(data);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
#if defined(__SAM3X8E__)			
			virtualProm[address] = *byteData;
			//logger() << F("RDB write address: ") << int(address) << " Data: " << (noOfBytes - address > 10 ? (char)*byteData : (int)virtualProm[address]) << L_endl;
#endif
			status |= eeprom().update(address, *byteData);
		}
		return status ? -1 : address;
	}

	int reader(int address, void * result, int noOfBytes) {
		if (address < RDB_START_ADDR || address + noOfBytes > RDB_START_ADDR + RDB_MAX_SIZE) {
			logger() << F("Illegal RDB read address: ") << int(address) << L_endl;
			return -1;
		}		
		uint8_t * byteData = static_cast<uint8_t *>(result);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
#if defined(__SAM3X8E__)
			//auto eepData = eeprom().read(address);
			//if (virtualProm[address] != eepData) {
			//	logger() << "Corrupted Virtual Data at addr: " << address << L_endl;
			//	*byteData = eepData;
			//	return address;
			//}
			*byteData = virtualProm[address];
#else
			* byteData = eeprom().read(address);
#endif
		}
		return address;
	}
}

using namespace	HeatingSystemSupport;
using namespace	Assembly;

HeatingSystem::HeatingSystem()
	: 
	_recover(i2C, STRATEGY_EPPROM_ADDR)
	, db(RDB_START_ADDR, writer, reader, VERSION)
	, _initialiser(*this)
	, _hs_queries(db)
	, _hs_datasets(*this, _hs_queries, _tempController)
	, _sequencer(_hs_queries)
	, _tempController(_recover, _hs_queries, _sequencer, _prog_registers, &_initialiser._resetI2C.hardReset.timeOfReset_mS)
	, mainDisplay(&_hs_queries.q_Displays)
	, localKeypad(KEYPAD_INT_PIN, KEYPAD_ANALOGUE_PIN, KEYPAD_REF_PIN, { RESET_LEDN_PIN, LOW })
	, remDispl{ {_recover, US_REMOTE_ADDRESS}, DS_REMOTE_ADDRESS, FL_REMOTE_ADDRESS } // must be same order as zones
	, remoteKeypadArr{ {remDispl[0].displ()},{remDispl[1].displ()},{remDispl[2].displ()} }
	, _mainConsoleChapters{ _hs_datasets, _tempController, *this}
	, _remoteConsoleChapters{ _hs_datasets }
	, _mainConsole(localKeypad, mainDisplay, _mainConsoleChapters)
	, _remoteLCDConsole{ {remoteKeypadArr[0], remDispl[0], _remoteConsoleChapters},{remoteKeypadArr[1], remDispl[1], _remoteConsoleChapters},{remoteKeypadArr[2], remDispl[2], _remoteConsoleChapters} }
	, remOLED_ConsoleArr{ {_recover, _prog_register_set},{_recover, _prog_register_set},{_recover, _prog_register_set} }
	, temporary_remoteTSArr{ {_recover, 0x36}, {_recover, 0x74}, {_recover, 0x70} }
	, temporary_mix_valve_TSArr{ {_recover, US_FLOW_TEMPSENS_ADDR}, {_recover, DS_FLOW_TEMPSENS_ADDR} }
	{
		i2C.setZeroCross({ ZERO_CROSS_PIN , LOW, INPUT_PULLUP });
		i2C.setZeroCrossDelay(ZERO_CROSS_DELAY);
		localKeypad.begin();
		HardwareInterfaces::localKeypad = &localKeypad;  // required by interrupt handler
		_initialiser.initializeRemoteConsoles();
		_initialiser.i2C_Test();
		_initialiser.postI2CResetInitialisation();
		//_mainConsoleChapters(0).rec_select();
		i2C.onReceive(_prog_register_set.receiveI2C);
		i2C.onRequest(_prog_register_set.requestI2C);
	}

void HeatingSystem::serviceTemperatureController() { // Called every Arduino loop
	if (_mainConsoleChapters.chapter() == 0) {
		if (_tempController.checkAndAdjust()) {
			if (ENABLE_OLED_REMOTES) {
				for (auto& remote : remOLED_ConsoleArr) {
					if (remote.getAddress() != 0x13) continue;
					remote.refreshRegisters();
				}
			}
		}
	}
}

void HeatingSystem::serviceConsoles() { // called every 50mS to respond to keys
	//Serial.println("HS.serviceConsoles");
	ui_yield();
	if (_mainConsole.processKeys()) _mainConsole.refreshDisplay();
	auto zoneIndex = 0;
	auto activeField = _remoteConsoleChapters.remotePage_c.activeUI();
	for (auto & remote : _remoteLCDConsole) {
		auto console_mode = remote.consoleMode();
		if (console_mode < 2) { ++zoneIndex; continue; }
		auto remoteTS_register = (RC_REG_MASTER_US_OFFSET + OLED_Master_Display::R_ROOM_TEMP) + (OLED_Master_Display::R_DISPL_REG_SIZE * zoneIndex);
		temporary_remoteTSArr[zoneIndex].readTemperature();
		auto roomTemp = temporary_remoteTSArr[zoneIndex].get_fractional_temp();
		_prog_registers.setRegister(remoteTS_register, roomTemp >> 8);
		_prog_registers.setRegister(remoteTS_register + 1, uint8_t(roomTemp));
		activeField->setFocusIndex(zoneIndex);

		if (remote.processKeys()) {
			remote.refreshDisplay();
		}
		++zoneIndex;
	}
}

void HeatingSystem::updateChangedData() {
	if (dataHasChanged && _mainConsoleChapters.chapter() == 0) {
		logger() << L_time << F("updateChangedData\n");
		_tempController.checkZones(true);
		dataHasChanged = false;
	}
#if defined ZPSIM
	else logger() << L_time << F("No ChangedData\n");
#endif
}

RelationalDatabase::RDB<Assembly::TB_NoOfTables> & HeatingSystem::getDB() { return db; }

