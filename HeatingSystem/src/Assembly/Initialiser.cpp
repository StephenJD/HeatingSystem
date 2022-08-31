#include "Initialiser.h"
#include "FactoryDefaults.h"
#include "..\HeatingSystem.h"
#include "..\HardwareInterfaces\A__Constants.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include <OLED_Thick_Display.h>
#include <Logging.h>
#include <RDB.h>
#include <Clock.h>
#include <EEPROM_RE.h>
#include <MemoryFree.h>
#include <I2C_Reset.h>
#include <Flag_Enum.h>
#include <Timer_mS_uS.h>

using namespace client_data_structures;
using namespace RelationalDatabase;
using namespace I2C_Talk_ErrorCodes;
using namespace HardwareInterfaces;
namespace arduino_logger {
	Logger& loopLogger();
}
using namespace arduino_logger;

namespace Assembly {
	using namespace RelationalDatabase;

	Initialiser::Initialiser(HeatingSystem & hs) 
		: 
		_resetI2C(hs._recover, _iniFunctor, _testDevices, { RESET_I2C_PIN , LOW }, { RESET_5vREF_PIN, LOW }, { RESET_LEDN_PIN , LOW})
		, _hs(hs)
		, _iniFunctor(*this)
		, _testDevices(*this)
	{
#ifdef ZPSIM
		//clock_().setTime(Date_Time::DateOnly{ 24,10,19 }, Date_Time::TimeOnly{ 11,40 }, 0);
		//clock_().setTime(Date_Time::DateOnly{ 15,9,19 }, Date_Time::TimeOnly{ 10,0 }, 5);
		//clock_().setTime(Date_Time::DateOnly{ 3,10,19 }, Date_Time::TimeOnly{ 10,0 }, 5);
#endif
		relayPort().setRecovery(hs._recover);
		ini_DB();
	}

	bool Initialiser::state_machine_OK() {
		auto iniOK = false;
		auto needs_ini = _iniState.firstNotSet();
		if (needs_ini <= REMOTE_CONSOLES) {
			loopLogger() << L_time << "needs_ini: " << needs_ini << " State: " << _iniState.flags() << L_endl;
		}
		switch (needs_ini) {
		case I2C_RESET:
			logger() << L_time << "DO_RESET..." << L_endl;
			_resetI2C(_hs.i2C, 0);
			_iniState.clear();
			_iniState.set(I2C_RESET);
			break;
		case TS:
			loopLogger() << L_time << "ESTABLISH_TS_COMS..." << L_endl;
			if (ini_TS() == _OK) {
				_iniState.set(TS);
			}
			else { _iniState.clear(I2C_RESET); }
			break;
		case POST_RESET_WARMUP:
			loopLogger() << L_time << "POST_RESET_WARMUP..." << L_endl;
			delay_mS(100);
			_iniState.set(POST_RESET_WARMUP, I2C_Recovery::HardReset::hasWarmedUp());
			break;
		case RELAYS:
			loopLogger() << L_time << "ESTABLISH_RELAY_COMS..." << L_endl;
			if (ini_relays() == _OK) {
				_iniState.set(RELAYS);
			} else { _iniState.clear(I2C_RESET); }
			break;
		case MIX_V:
			loopLogger() << L_time << "ESTABLISH_MIXV_COMMS..." << L_endl;
			if (post_initialize_MixV() == _OK) {
				_iniState.set(MIX_V);
			} else { _iniState.clear(I2C_RESET); }
			break;
		case REMOTE_CONSOLES:
			loopLogger() << L_time << "ESTABLISH_REMOTE_CONSOLE_COMS..." << L_endl;
			if (post_initialize_Thick_Consoles() == _OK) {
				_iniState.set(REMOTE_CONSOLES);
			} else { _iniState.clear(I2C_RESET); }
			break;
		}

		return _iniState.all();
	}

	bool Initialiser::ini_DB() {
		logger() << F("ini_DB") << L_endl;

		if (!_hs.db.passwordOK()) {
			logger() << F("  DB PW Failed") << L_endl;
			setFactoryDefaults(_hs.db, VERSION);
		}
		auto dbFailed = false;
		for (auto & table : _hs.db) {
			if (!table.isOpen()) {
				logger() << F("  Table not open at ") << table.tableID() << L_endl;
				dbFailed = true;
				break;
			}
		}
		if (dbFailed) {
			logger() << F("  DB Failed") << L_endl;
			setFactoryDefaults(_hs.db, VERSION);
		}
		return dbFailed == false;
	}

	uint8_t Initialiser::ini_TS() {
		auto status = _OK;
		logger() << L_time << F("ini_TS...") << L_flush;
		for (auto& ts : _hs._tempController.tempSensorArr) {
			if (ts.testDevice() != _OK) {
				auto speedTest = I2C_SpeedTest{ ts };
				speedTest.fastest();
				status |= speedTest.error();
			}
		}	
		logger() << L_time << F("ini_TS-OK") << L_flush;
		return status;
	}

	uint8_t Initialiser::ini_relays() {
		logger() << L_time << F("ini_relays") << L_flush;
		if (static_cast<RelaysPort&>(relayController()).isUnrecoverable()) I2C_Recovery::HardReset::arduinoReset("RelayController");
		return relayPort().initialiseDevice();
	}

	void Initialiser::initialize_Thick_Consoles() {
		i2c_registers::RegAccess(_hs._prog_register_set).set(R_SLAVE_REQUESTING_INITIALISATION, ALL_REQUESTING);
		auto consoleIndex = 0;
		for (auto& rc : _hs.thickConsole_Arr) {
			logger() << L_time << F("RemConsole query[] ") << consoleIndex << L_endl;
			Answer_R<R_Display> display = _hs._hs_queries.q_Consoles[consoleIndex + 1];
			auto modeFlags = display.rec().timeout;
			logger() << L_time << F("set_console_mode :") << modeFlags << L_endl;
			rc.initialise(consoleIndex, REMOTE_CONSOLE_ADDR[consoleIndex], REMOTE_ROOM_TS_ADDR[consoleIndex]
				, hs().tempController().towelRailArr[consoleIndex], hs().tempController().zoneArr[Z_DHW]
				, hs().tempController().zoneArr[consoleIndex]
				, modeFlags);
			++consoleIndex;
		}
	}

	uint8_t Initialiser::post_initialize_MixV() {
		uint8_t status = 0;
		for (auto& mixValveControl : hs().tempController().mixValveControllerArr) {
			loopLogger() << L_time << F("post_ini_MixV :") << mixValveControl.index() << L_endl;
			if (mixValveControl.isUnrecoverable()) { // I2C_Recover_Retest::_deviceWaitingOnFailureFor10Mins set.
				loopLogger() << "isUnrecoverable" << L_endl;
				I2C_Recovery::HardReset::arduinoReset("MixValveController");
			}
			status |= mixValveControl.sendSlaveIniData(*i2c_registers::RegAccess(_hs._prog_register_set).ptr(R_SLAVE_REQUESTING_INITIALISATION));
		}
		return status;
	}
	
	uint8_t Initialiser::post_initialize_Thick_Consoles() {
		uint8_t status = 0;
		for (auto& remote : _hs.thickConsole_Arr) {
			if (remote.isUnrecoverable()) I2C_Recovery::HardReset::arduinoReset("RemoteConsoles");
			status |= remote.sendSlaveIniData(*i2c_registers::RegAccess(_hs._prog_register_set).ptr(R_SLAVE_REQUESTING_INITIALISATION));
		}
		for (auto& remote : _hs.thickConsole_Arr) {
			remote.refreshRegistersOK();
			auto timeout = Timer_mS(100);
			while (!timeout && remote.registers().get(OLED_Thick_Display::R_ROOM_TEMP) == 0);
#ifndef ZPSIM
			if (timeout) 
				requiresINI(I2C_RESET);
#endif
		}
		return status;
	}		

	uint8_t Initialiser::i2C_Test() {
		uint8_t status = _testDevices.speedTestDevices();
		status |= _testDevices.testRelays();
		return status;
	}

	uint8_t Initialiser::notify_reset() {
		logger() << "Set Reset-Flag" << L_endl;
		_iniState.clear();
		_iniState.set(I2C_RESET);
		return _OK;
	}

	I_I2Cdevice_Recovery & Initialiser::getDevice(uint8_t deviceAddr) {
		if (deviceAddr == IO8_PORT_OptCoupl) return relayPort();
		else if (deviceAddr == MIX_VALVE_I2C_ADDR) return hs().tempController().mixValveControllerArr[0];
		else if (deviceAddr >= US_CONSOLE_I2C_ADDR && deviceAddr <= FL_CONSOLE_I2C_ADDR) {
			return hs().thickConsole_Arr[deviceAddr - US_CONSOLE_I2C_ADDR];
		} 
		else {
			for (auto & ts : hs().tempController().tempSensorArr) {
				if (ts.getAddress() == deviceAddr) return ts;
			}
			return hs().tempController().tempSensorArr[0];
		}
	}

	uint8_t IniFunctor:: operator()() { // inlining creates circular dependancy
		return _ini->notify_reset();
	}
}

