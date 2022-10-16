#include "HeatingSystem.h"
#include "Assembly\FactoryDefaults.h"
#include "HardwareInterfaces\A__Constants.h"
#include "LCD_UI\A_Top_UI.h"
#include <I2C_Reset.h>
#include <EEPROM_RE.h>
#include <Clock.h>
#include <Logging_Ram.h>
#include <MemoryFree.h>
#include <Mega_Due.h>

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif

using namespace client_data_structures;
using namespace RelationalDatabase;
using namespace HardwareInterfaces;
using namespace arduino_logger;
using namespace I2C_Talk_ErrorCodes;

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
namespace arduino_logger {
	Logger& zTempLogger();
	Logger& profileLogger();
	Logger& loopLogger();
}

void printFileHeadings() {
	zTempLogger()
		<< F("Time") << L_tabs << F("Zone")
		<< F("PreReq")
		<< F("PreIs")
		<< F("FlowReq")
		<< F("FlowIs")
		<< F("UsedRatio")
		<< F("Is")
		<< F("Ave")
		<< F("AvePer")
		<< F("CoolPer")
		<< F("Error16ths")
		<< F("Outside")
		<< F("PreheatMins")
		<< F("ControlledBy")
		<< F("IsOn")
		<< L_endl;
	profileLogger() << "Time\tZone\tReq\tIs\tState\tTime\tPos\tAdjust\tRatio\tPSU_V\n";
}

void flushLogs() {
	logger().flush();
	loopLogger() << "logger flush done" << L_endl;
	zTempLogger().flush();
	loopLogger() << "zTempLogger flush done" << L_endl;
	profileLogger().flush();
	loopLogger() << "profileLogger flush done" << L_endl;
}

HeatingSystem::HeatingSystem()
	: 
	_recover(i2C, STRATEGY_EPPROM_ADDR)
	, db(RDB_START_ADDR, writer, reader, VERSION)
	, _initialiser(*this)
	, _hs_queries(db)
	, _hs_datasets(*this, _hs_queries, _tempController)
	, _sequencer(_hs_queries)
	, _tempController(_recover, _hs_queries, _sequencer, _prog_registers)
	, mainDisplay(&_hs_queries.q_Displays)
	, localKeypad(KEYPAD_INT_PIN, KEYPAD_ANALOGUE_PIN, KEYPAD_REF_PIN, { RESET_LEDN_PIN, LOW })
	, thickConsole_Arr{ {_recover, _prog_register_set},{_recover, _prog_register_set},{_recover, _prog_register_set} }
	, _mainConsoleChapters{ _hs_datasets, _tempController, *this}
	, _mainConsole(localKeypad, mainDisplay, _mainConsoleChapters)
	{
		i2C.setZeroCross({ ZERO_CROSS_PIN , LOW, INPUT_PULLUP });
		i2C.setZeroCrossDelay(ZERO_CROSS_DELAY);
		localKeypad.begin();
		HardwareInterfaces::localKeypad = &localKeypad;  // required by interrupt handler
		_initialiser.initialize_Thick_Consoles();
		_initialiser.resetDone(_initialiser.i2C_Test() == _OK);
		i2C.onReceive(_prog_register_set.receiveI2C);
		i2C.onRequest(_prog_register_set.requestI2C);
		_state = CHECK_I2C_COMS;
	}

void HeatingSystem::run_stateMachine() {
	//TODO: Profile / preheat not done immedietly. Pump shows on for a while.
	//logger() << L_cout << "Ram: " << static_cast<RAM_Logger&>(loopLogger()).c_str() << L_endl;
	//logger() << L_cout << "Ram End" << L_endl;
	loopLogger().begin();
	loopLogger() << "State: " << _state << L_endl;
	switch (_state) {
	case CHECK_I2C_COMS: // once per second
		if (_initialiser.state_machine_OK()) _state = SERVICE_CONSOLES;
		serviceMainConsole();
		break;
	case START_NEW_DAY:
		loopLogger() << L_time << "START_NEW_DAY" << L_endl;
		printFileHeadings();
		[[fallthrough]];
	case SERVICE_SEQUENCER:
		loopLogger() << L_time << "SERVICE_SEQUENCER" << L_endl;
		_tempController.checkZoneRequests(true); // must be called once every 10 mins and when data changes
		[[fallthrough]];
	case SERVICE_BACKBOILER:
		loopLogger() << L_time << "SERVICE_BACKBOILER" << L_endl;
		_tempController.backBoiler.check();
		loopLogger() << L_time << "SERVICE_BACKBOILER_Done" << L_endl;
		[[fallthrough]];
	case SERVICE_TEMP_CONTROLLER: {
			//logger() << L_cout << "Ram: " << static_cast<RAM_Logger&>(loopLogger()).c_str() << L_endl;
			//logger() << L_cout << "Ram End" << L_endl;
			//loopLogger().begin();
			loopLogger() << L_time << "SERVICE_TEMP_CONTROLLER" << L_endl;
			logger() << L_time << "SERVICE_TEMP_CONTROLLER state(252): " << _initialiser.iniState().flags() << L_endl;
			//thickConsole_Arr[1].sendSlaveIniData(RC_US_REQUESTING_INI << 1);
			auto status = ALL_OK;
			if (_mainConsoleChapters.chapter() == 0) status = _tempController.checkAndAdjust();
			logger() << L_time << "...checkAndAdjust done: " << status /*<< " iniState: " << _initialiser.iniState().flags()*/ << L_endl;
			serviceConsoles_OK();
			logger() << "\t...refresh all Registers done: " << status /*<< " iniState: " << _initialiser.iniState().flags()*/ << L_endl;
			loopLogger() << "...refresh all Registers done: " << status << L_endl;
			switch (status) {
			case TS_FAILED:
				loopLogger() << L_time << "TS-Failed" << L_endl;
				logger() << L_time << "TS-Failed" << L_flush;
				_initialiser.requiresINI(Initialiser::TS);
				break;
			case MV_FAILED:
				loopLogger() << L_time << "MV-Failed" << L_endl;
				logger() << L_time << "MV-Failed" << L_flush;
				_initialiser.requiresINI(Initialiser::MIX_V);
				break;
			case RELAYS_FAILED:
				loopLogger() << L_time << "Relay-Failed" << L_endl;
				logger() << L_time << "Relay-Failed" << L_flush;
				_initialiser.requiresINI(Initialiser::RELAYS);
				break;
			}
			loopLogger() << "flushLogs..." << L_endl;
			flushLogs();
			loopLogger() << "flushLogs done" << L_endl;
			_state = CHECK_I2C_COMS;
		}	
		break;
	case SERVICE_CONSOLES:
		static uint8_t lastSec = clock_().seconds() - 1;
		switch (clock_().isNewPeriod(lastSec)) {
		case Clock::NEW_DAY:
			_state = START_NEW_DAY;
			return;
		case Clock::NEW_HR:
			//loopLogger().activate(clock_().hrs() % 2 ? true : false);
		case Clock::NEW_MIN10:
			_state = SERVICE_SEQUENCER;
			return;
		case Clock::NEW_MIN:
			_state = SERVICE_BACKBOILER;
			return;
		case Clock::NEW_SEC10:
		case Clock::NEW_SEC:
			_state = SERVICE_TEMP_CONTROLLER;
			return;
#ifdef ZPSIM
		default:
			_state = SERVICE_TEMP_CONTROLLER;
#endif	
		}
		serviceConsoles_OK();
		break;
	}
	//loopLogger().flush();
}

bool HeatingSystem::serviceConsoles_OK() {  // called every 50mS to respond to keys, also called by yield()
	bool rc_OK = true;
	//loopLogger() << L_time << "serviceConsoles?" << L_endl;
	if (serviceMainConsole()) {
		for (auto& remote : thickConsole_Arr) {
			if (!remote.refreshRegistersOK()) {
				rc_OK = false;
				loopLogger() << L_time << "RC-Failed" << L_endl;
				logger() << L_time << "RC-Failed" << L_flush;
				_initialiser.requiresINI(Initialiser::REMOTE_CONSOLES);
			}
			logger() << L_time <<"refresh RC's " << (rc_OK? "OK":"Bad") /*<< " iniState: " << _initialiser.iniState().flags()*/ << L_endl;
		}
	}
	if (dataHasChanged) {
		loopLogger() << L_time << "...updateChangedData" << L_endl;
		updateChangedData();
		loopLogger() << L_time << "Done updateChangedData" << L_endl;
		dataHasChanged = false;
		//logger() << "\tupdateChangedData iniState: " << _initialiser.iniState().flags() << L_endl;
	}
	return rc_OK;
}

bool HeatingSystem::serviceMainConsole() {
	if (consoleDataHasChanged()) {
		logger() << L_time << "refreshDisplay s" << (micros() / 1000000) % 10 << L_endl;
		_mainConsole.refreshDisplay();
		return true;
	}
	return false;
}

bool HeatingSystem::consoleDataHasChanged() {
	bool displayHasChanged = _mainConsole.processKeys();
	for (auto& remote : thickConsole_Arr) {
		displayHasChanged |= remote.hasChanged();
	}
	return displayHasChanged;
}

void HeatingSystem::updateChangedData() {
	if (_mainConsoleChapters.chapter() == 0) {
		loopLogger() << L_time << F("updateChangedData\n");
		_tempController.checkZoneRequests(true);
	}
}

RelationalDatabase::RDB<Assembly::TB_NoOfTables> & HeatingSystem::getDB() { return db; }

