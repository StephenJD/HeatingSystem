#include "Initialiser.h"
#include "FactoryDefaults.h"
#include "..\HeatingSystem.h"
#include "..\HardwareInterfaces\I2C_Comms.h"
#include "..\HardwareInterfaces\A__Constants.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include <Logging.h>
#include <RDB.h>
#include <../Clock/Clock.h>
#include <EEPROM.h>
#include <MemoryFree.h>

using namespace client_data_structures;
using namespace RelationalDatabase;
using namespace I2C_Talk_ErrorCodes;
using namespace HardwareInterfaces;
using namespace arduino_logger;

namespace Assembly {
	using namespace RelationalDatabase;

	Initialiser::Initialiser(HeatingSystem & hs) 
		: 
		_resetI2C(hs._recover, _iniFunctor, _testDevices, { RESET_OUT_PIN , LOW})
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

		logger() << F("Initialiser Construction") << L_endl;
		if (!_hs.db.passwordOK()) {
			logger() << F("  Initialiser PW Failed") << L_endl;
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
			logger() << F("  dbFailed") << L_endl;
			setFactoryDefaults(_hs.db, VERSION);
		}
		logger() << F("  Initialiser Constructed") << L_endl;
	}

	void Initialiser::initializeRemoteConsoles() {
		_hs._prog_register_set.setRegister(R_SLAVE_REQUESTING_INITIALISATION, ALL_REQUESTING);
		auto consoleIndex = 0;
		for (auto& rc : _hs.remOLED_ConsoleArr) {
			rc.initialise(consoleIndex, REMOTE_CONSOLE_ADDR[consoleIndex], REMOTE_ROOM_TS_ADDR[consoleIndex], hs().tempController().towelRailArr[consoleIndex], hs().tempController().thermalStore, hs().tempController().zoneArr[consoleIndex], _resetI2C.hardReset.timeOfReset_mS);
			++consoleIndex;
		}
	}

	uint8_t Initialiser::i2C_Test() {
		uint8_t status = _OK;
		status = _testDevices.speedTestDevices();
		_testDevices.testRelays();
		return status;
	}

	uint8_t Initialiser::postI2CResetInitialisation() {
		_resetI2C.hardReset.initialisationRequired = false;
		uint8_t status =  temporary_initialiseTempSensors()
			| relayPort().initialiseDevice()
			| temporary_initialiseRemoteDisplays()
			/* | initialiseMixValveController() */ ;
		for (auto& mixValve : hs().tempController().mixValveControllerArr) { mixValve.sendSlaveIniData(); }
		if (status != _OK) logger() << F("  Initialiser::i2C_Test postI2CResetInitialisation failed") << L_endl;
		else logger() << F("  Initialiser::postI2CResetInitialisation OK\n");
		return status;
	}

	uint8_t Initialiser::temporary_initialiseTempSensors() {
		// Set room-sensors to high-res
		logger() << F("\nSet room-sensors to high-res") << L_endl;
		return	_hs.temporary_remoteTSArr[0].setHighRes()
			| _hs.temporary_remoteTSArr[1].setHighRes()
			| _hs.temporary_remoteTSArr[2].setHighRes();
	}

	uint8_t Initialiser::temporary_initialiseRemoteDisplays() {
		uint8_t failed = 0;
		logger() << L_time << F("temporary_initialiseRemoteDisplays()") << L_endl;
		for (auto & rd : _hs.remDispl) {
			failed |= rd.initialiseDevice();
		}

		auto index = 1;
		for (auto& kb : _hs.remoteKeypadArr) {
			Answer_R<R_Display> display = _hs._hs_queries.q_Consoles[index];
			kb.popKey();
			kb.clearKeys();
			kb.setWakeTime(display.rec().timeout);
			++index;
		}
		logger() << F("\tinitialiseRemoteDisplays() done") << L_endl;
		return failed;
	}

	//uint8_t Initialiser::initialiseMixValveController() {
	//	logger() << F("\nSend Req Temp") << L_endl;
	//	_hs.tempController().mixValveControllerArr[M_DownStrs].sendRequestFlowTemp();
	//	_hs.tempController().mixValveControllerArr[M_UpStrs].sendRequestFlowTemp();
	//	return _OK;
	//}


	I_I2Cdevice_Recovery & Initialiser::getDevice(uint8_t deviceAddr) {
		if (deviceAddr == IO8_PORT_OptCoupl) return relayPort();
		else if (deviceAddr == MIX_VALVE_I2C_ADDR) return hs().tempController().mixValveControllerArr[0];
		else if (deviceAddr >= 0x24 && deviceAddr <= 0x26) {
			return hs().remDispl[deviceAddr-0x24];
		}
		else {
			for (auto & ts : hs().tempController().tempSensorArr) {
				if (ts.getAddress() == deviceAddr) return ts;
			}
			return hs().tempController().tempSensorArr[0];
		}
	}

	uint8_t IniFunctor:: operator()() { // inlining creates circular dependancy
		return _ini->postI2CResetInitialisation();
	}
}

