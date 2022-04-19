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
	profileLogger() << "Time\tZone\tReq\tIs\tState\tTime\tPos\tRatio\tFromP\tFromT\n";
}

void flushLogs() {
	logger().flush();
	zTempLogger().flush();
	profileLogger().flush();
}

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
	, _mainConsoleChapters{ _hs_datasets, _tempController, *this}
	, _mainConsole(localKeypad, mainDisplay, _mainConsoleChapters)
	, thickConsole_Arr{ {_recover, _prog_register_set},{_recover, _prog_register_set},{_recover, _prog_register_set} }
	{
		i2C.setZeroCross({ ZERO_CROSS_PIN , LOW, INPUT_PULLUP });
		i2C.setZeroCrossDelay(ZERO_CROSS_DELAY);
		localKeypad.begin();
		HardwareInterfaces::localKeypad = &localKeypad;  // required by interrupt handler
		_initialiser.initialize_Thick_Consoles();
		_initialiser.i2C_Test();
		i2C.onReceive(_prog_register_set.receiveI2C);
		i2C.onRequest(_prog_register_set.requestI2C);
		_initialiser.postI2CResetInitialisation();
}

void HeatingSystem::run_stateMachine() {
	//TODO: Profile / preheat not done immedietly. Pump shows on for a while.
	loopLogger().begin();
	switch (_state) {
	case ESTABLISH_TS_COMS:
		loopLogger() << L_time << "ESTABLISH_TS_COMS" << L_endl;
		if (!_tempController.readTemperaturesOK()) break;
		_state = POST_RESET;
		break;
	case ESTABLISH_RELAY_COMS:
		loopLogger() << L_time << "ESTABLISH_RELAY_COMS" << L_endl;
		if (static_cast<RelaysPort&>(relayController()).isUnrecoverable()) HardReset::arduinoReset("RelayController");
		_state = POST_RESET;
		break;
	case ESTABLISH_MIXV_COMMS:
		loopLogger() << L_time << "ESTABLISH_MIXV_COMMS" << L_endl;
		for (auto& mixValveControl : _tempController.mixValveControllerArr) {
			if (mixValveControl.isUnrecoverable()) HardReset::arduinoReset("MixValveController");
		}
		_state = POST_RESET;
		break;
	case ESTABLISH_REMOTE_CONSOLE_COMS:
		loopLogger() << L_time << "ESTABLISH_REMOTE_CONSOLE_COMS" << L_endl;
		for (auto& remote : thickConsole_Arr) {
			if (remote.isUnrecoverable()) HardReset::arduinoReset("RemoteConsoles");
		}
		_state = POST_RESET;
		break;
	case POST_RESET:
		loopLogger() << L_time << "POST_RESET" << L_endl;
		_initialiser.postI2CResetInitialisation();
		_state = SERVICE_TEMP_CONTROLLER;
		break;
	case START_NEW_DAY:
		loopLogger() << L_time << "START_NEW_DAY" << L_endl;
		printFileHeadings();
		[[fallthrough]];
	case SERVICE_SEQUENCER:
		loopLogger() << L_time << "SERVICE_SEQUENCER" << L_endl;
		[[fallthrough]];
	case SERVICE_ZONES:
		loopLogger() << L_time << "SERVICE_ZONES" << L_endl;
		_tempController.checkZoneRequests(_state == SERVICE_SEQUENCER);
		_tempController.backBoiler.check();
		loopLogger() << L_time << "SERVICE_ZONES_Done" << L_endl;
		[[fallthrough]];
	case SERVICE_TEMP_CONTROLLER: {
			loopLogger() << L_time << "SERVICE_TEMP_CONTROLLER" << L_endl;
			auto status = ALL_OK;
			if (_mainConsoleChapters.chapter() == 0) status = _tempController.checkAndAdjust();
			loopLogger() << L_time << "...checkAndAdjust done" << L_endl;
			for (auto& remote : thickConsole_Arr) {
				if (!remote.refreshRegistersOK()) status = RC_FAILED;
			}
			loopLogger() << "...refreshRegistersOK done" << L_endl;
			switch (status) {
			case TS_FAILED:
				_state = ESTABLISH_TS_COMS;
				break;
			case MV_FAILED:
				loopLogger() << L_time << "_resetI2C-MixV" << L_endl;
				_initialiser._resetI2C(i2C, MIX_VALVE_I2C_ADDR);
				loopLogger() << L_time << "..._resetI2C-MixV done" << L_endl;
				_state = ESTABLISH_MIXV_COMMS;
				break;
			case RC_FAILED:
				loopLogger() << L_time << "_resetI2C-RC" << L_endl;
				_initialiser._resetI2C(i2C, US_CONSOLE_I2C_ADDR);
				loopLogger() << L_time << "..._resetI2C-RC done" << L_endl;
				_state = ESTABLISH_REMOTE_CONSOLE_COMS;
				break;
			case RELAYS_FAILED:
				loopLogger() << L_time << "_resetI2C-Relays" << L_endl;
				_initialiser._resetI2C(i2C, IO8_PORT_OptCoupl);
				loopLogger() << L_time << "..._resetI2C-Relays done" << L_endl;
				_state = ESTABLISH_RELAY_COMS;
				break;
			default:
				_state = SERVICE_CONSOLES;
			}
			flushLogs();
		}	
		break;
	case SERVICE_CONSOLES:
		if (!serviceConsolesOK()) {
			_state = ESTABLISH_REMOTE_CONSOLE_COMS;
		}
		else {
			if (dataHasChanged) {
				loopLogger() << L_time << "...updateChangedData" << L_endl;
				updateChangedData();
				loopLogger() << L_time << "Done updateChangedData" << L_endl;
				dataHasChanged = false;
				_state = SERVICE_TEMP_CONTROLLER;
			}
			else {
				static uint8_t lastSec = clock_().seconds() - 1;
				switch (clock_().isNewPeriod(lastSec)) {
				case Clock::NEW_DAY:
					_state = START_NEW_DAY;
					break;
				case Clock::NEW_HR:
				case Clock::NEW_MIN10:
					_state = SERVICE_SEQUENCER;
					break;
				case Clock::NEW_MIN:
					_state = SERVICE_ZONES;
					break;
				case Clock::NEW_SEC10:
				case Clock::NEW_SEC:
					_state = SERVICE_TEMP_CONTROLLER;
					break;
#ifdef ZPSIM
				default:
					_state = SERVICE_TEMP_CONTROLLER;
#endif	
				}
			}
		}
		break;
	}
	//logger() << "State: " << _state << L_endl;
}

bool HeatingSystem::serviceConsolesOK() {  // called every 50mS to respond to keys, also called by yield()
	auto rc_OK = true;
	if (consoleDataHasChanged()) {
		_mainConsole.refreshDisplay();
		//loopLogger() << L_time << "Done refresh_mainConsole" << L_endl;
		for (auto& remote : thickConsole_Arr) {
			rc_OK &= remote.refreshRegistersOK();
		}
		loopLogger() << L_time << "Done refreshRegistersOK" << L_endl;
	}
	return rc_OK;
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

