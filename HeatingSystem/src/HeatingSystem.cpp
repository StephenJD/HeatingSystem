#include "HeatingSystem.h"
#include "Assembly\FactoryDefaults.h"
#include "HardwareInterfaces\I2C_Comms.h"
#include "HardwareInterfaces\A__Constants.h"
#include "LCD_UI\A_Top_UI.h"
#include <EEPROM.h>
#include <MemoryFree.h>

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif

using namespace client_data_structures;
using namespace RelationalDatabase;
using namespace HardwareInterfaces;

//extern Remote_display * hall_display;
//extern Remote_display * flat_display;
//extern Remote_display * us_display;

namespace HeatingSystemSupport {
	// Due has 96kB RAM
	uint8_t virtualProm[RDB_START_ADDR + RDB_MAX_SIZE];

	void initialise_virtualROM() {
#if defined(__SAM3X8E__)		
		logger() << F("  virtualROM...\n");
		eeprom().readEP(RDB_START_ADDR, RDB_MAX_SIZE, virtualProm + RDB_START_ADDR);
		logger() << F("  virtualROM Complete. Size: ") << sizeof(virtualProm) << L_endl;
#endif
	}

	int writer(int address, const void * data, int noOfBytes) {
		//if (address == 495) {
		//	auto debug = false;
		//}
		if (address < RDB_START_ADDR || address + noOfBytes > RDB_START_ADDR + RDB_MAX_SIZE) {
			logger() << F("Illegal RDB write address: ") << int(address) << L_endl;
			return address;
		}
		//logger() << F("RDB write address: ") << int(address) << L_endl;
		const unsigned char * byteData = static_cast<const unsigned char *>(data);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
#if defined(__SAM3X8E__)			
			virtualProm[address] = *byteData;
#endif
			eeprom().update(address, *byteData);
		}
		return address;
	}

	int reader(int address, void * result, int noOfBytes) {
		if (address < RDB_START_ADDR || address + noOfBytes > RDB_START_ADDR + RDB_MAX_SIZE) {
			logger() << F("Illegal RDB read address: ") << int(address) << L_endl;
			return address;
		}		
		uint8_t * byteData = static_cast<uint8_t *>(result);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
#if defined(__SAM3X8E__)
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
	, _tempController(_recover, db, &_initialiser._resetI2C.hardReset.timeOfReset_mS)
	, _hs_queries(db, _tempController)
	, mainDisplay(&_hs_queries._q_displays)
	, localKeypad(KEYPAD_INT_PIN, KEYPAD_ANALOGUE_PIN, KEYPAD_REF_PIN, { RESET_LEDN_PIN, LOW })
	, remoteKeypad{ {remDispl[0].displ()},{remDispl[0].displ()},{remDispl[0].displ()} }
	, remDispl{ {_recover, US_REMOTE_ADDRESS}, FL_REMOTE_ADDRESS, DS_REMOTE_ADDRESS }
	, _mainConsoleChapters{ _hs_queries, _tempController, *this}
	, _remoteConsoleChapters{_hs_queries}
	, _sequencer(_hs_queries, _tempController)
	, _mainConsole(localKeypad, mainDisplay, _mainConsoleChapters)
	, _remoteConsole{ {remoteKeypad[0], remDispl[0], _remoteConsoleChapters},{remoteKeypad[1], remDispl[1], _remoteConsoleChapters},{remoteKeypad[2], remDispl[2], _remoteConsoleChapters} }
	{
		i2C.setZeroCross({ ZERO_CROSS_PIN , LOW, INPUT_PULLUP });
		i2C.setZeroCrossDelay(ZERO_CROSS_DELAY);
		localKeypad.begin();
		HardwareInterfaces::localKeypad = &localKeypad;  // required by interrupt handler
		_initialiser.i2C_Test();
	}

void HeatingSystem::serviceTemperatureController() {
	if (_mainConsoleChapters.chapter() == 0) {
		_tempController.checkAndAdjust();
	}
}

void HeatingSystem::serviceConsoles() {
	_mainConsole.processKeys();
	auto i = 0;
	
	for (auto & remote : _remoteConsole) {
		auto activeField = _remoteConsoleChapters.remotePage_c.activeUI();
		//cout << "\nRemote activeField: " << ui_Objects()[(long)activeField->get()] << endl;
		activeField->setFocusIndex(i);
		//logger() << "Remote PageGroup focus: " << _remoteConsoleChapters._remote_page_group_c.focusIndex() << L_endl;
		remote.processKeys();
		++i;
		if (i == 2) ++i;
	}
}

void HeatingSystem::serviceProfiles() { _sequencer.getNextEvent(); }

void HeatingSystem::notifyDataIsEdited() {
	if (_mainConsoleChapters.chapter() == 0) {
		logger() << L_time << F("notifyDataIsEdited\n");
		_tempController.checkZones();
		logger() << L_time << F("recheckNextEvent...\n");
		_sequencer.recheckNextEvent();
	}
}

RelationalDatabase::RDB<Assembly::TB_NoOfTables> & HeatingSystem::getDB() { return db; }

