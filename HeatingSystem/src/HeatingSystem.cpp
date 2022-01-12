#include "HeatingSystem.h"
#include "Assembly\FactoryDefaults.h"
#include "HardwareInterfaces\I2C_Comms.h"
#include "HardwareInterfaces\A__Constants.h"
#include "LCD_UI\A_Top_UI.h"
#include <EEPROM_RE.h>
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

	int writer(int address, const void * data, int noOfBytes) { // return next address, or 0 for failure
		//if (address == 495) {
		//	auto debug = false;
		//}
		uint8_t status = 0;
		if (address < RDB_START_ADDR || address + noOfBytes > RDB_START_ADDR + RDB_MAX_SIZE) {
			logger() << F("Illegal RDB write address: ") << int(address) << L_endl;
			return status;
		}
		const unsigned char * byteData = static_cast<const unsigned char *>(data);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
#if defined(__SAM3X8E__)			
			virtualProm[address] = *byteData;
			//logger() << F("RDB write address: ") << int(address) << " Data: " << (noOfBytes - address > 10 ? (char)*byteData : (int)virtualProm[address]) << L_endl;
#endif
			status |= eeprom().update(address, *byteData);
		}
		return status ? 0 : address;
	}

	int reader(int address, void * result, int noOfBytes) { // return next address, or 0 for failure
		if (address < RDB_START_ADDR || address + noOfBytes > RDB_START_ADDR + RDB_MAX_SIZE) {
			logger() << F("Illegal RDB read address: ") << int(address) << L_endl;
			return 0;
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
			if (eeprom().readErr()) return 0;
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
	, _thinConsole_Chapters{ _hs_datasets }
	, _mainConsole(localKeypad, mainDisplay, _mainConsoleChapters)
	, _thinConsole_Arr{ {remoteKeypadArr[0], remDispl[0], _thinConsole_Chapters},{remoteKeypadArr[1], remDispl[1], _thinConsole_Chapters},{remoteKeypadArr[2], remDispl[2], _thinConsole_Chapters} }
	, thickConsole_Arr{ {_recover, _prog_register_set},{_recover, _prog_register_set},{_recover, _prog_register_set} }
	{
		i2C.setZeroCross({ ZERO_CROSS_PIN , LOW, INPUT_PULLUP });
		i2C.setZeroCrossDelay(ZERO_CROSS_DELAY);
		localKeypad.begin();
		HardwareInterfaces::localKeypad = &localKeypad;  // required by interrupt handler
		_initialiser.initialize_Thick_Consoles();
		_initialiser.i2C_Test();
		_initialiser.postI2CResetInitialisation();
		//_mainConsoleChapters(0).rec_select();
		i2C.onReceive(_prog_register_set.receiveI2C);
		i2C.onRequest(_prog_register_set.requestI2C);
		serviceTemperatureController();
	}

void HeatingSystem::serviceTemperatureController() { // Called every Arduino loop
#ifdef ZPSIM
	{
#else
	if (_tempController.isNewSecond()) {
#endif
		if (_mainConsoleChapters.chapter() == 0) _tempController.checkAndAdjust();
		for (auto& remote : thickConsole_Arr) {
			remote.refreshRegisters();
		}
	}
}

void HeatingSystem::serviceConsoles() { // called every 50mS to respond to keys
	//Serial.println("HS.serviceConsoles");
	ui_yield();
	bool displayHasChanged = _mainConsole.processKeys();
	auto consoleIndex = 0;
	auto activeField = _thinConsole_Chapters.remotePage_c.activeUI();
	for (auto & remote : _thinConsole_Arr) {
		auto consoleMode = OLED_Thick_Display::ModeFlags(remote.consoleMode());
		if (consoleMode.is(OLED_Thick_Display::e_LCD)) {
			activeField->setFocusIndex(consoleIndex);
			if (remote.processKeys() || displayHasChanged) {
				remote.refreshDisplay();
				displayHasChanged = true;
			}
		}
		else {
			displayHasChanged |= thickConsole_Arr[consoleIndex].hasChanged();
		}
		++consoleIndex;
	}
	if (displayHasChanged) _mainConsole.refreshDisplay();
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

