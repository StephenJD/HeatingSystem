#include "Initialiser.h"
#include "FactoryDefaults.h"
#include "..\HeatingSystem.h"
#include "..\HardwareInterfaces\I2C_Comms.h"
#include "..\HardwareInterfaces\A__Constants.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include <OLED_Thick_Display.h>
#include <Logging.h>
#include <RDB.h>
#include <Clock.h>
#include <EEPROM_RE.h>
#include <MemoryFree.h>
#include <Flag_Enum.h>

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

	void Initialiser::initialize_Thick_Consoles() {
		//i2c_registers::RegAccess(_hs._prog_register_set).set(R_SLAVE_REQUESTING_INITIALISATION, ALL_REQUESTING);
		auto consoleIndex = 0;
		for (auto& rc : _hs.thickConsole_Arr) {
			logger() << L_time << F("RemConsole query[] ") << consoleIndex << L_endl;
			Answer_R<R_Display> display = _hs._hs_queries.q_Consoles[consoleIndex + 1];
			auto modeFlags = display.rec().timeout;
			logger() << L_time << F("set_console_mode :") << modeFlags << L_endl;
			rc.initialise(consoleIndex, REMOTE_CONSOLE_ADDR[consoleIndex], REMOTE_ROOM_TS_ADDR[consoleIndex]
				, hs().tempController().towelRailArr[consoleIndex], hs().tempController().zoneArr[Z_DHW]
				, hs().tempController().zoneArr[consoleIndex], _resetI2C.hardReset.timeOfReset_mS
				, modeFlags);
			++consoleIndex;
		}
	}	
	
	uint8_t Initialiser::post_initialize_Thick_Consoles() {
		uint8_t status = 0;
		i2c_registers::RegAccess(_hs._prog_register_set).set(R_SLAVE_REQUESTING_INITIALISATION, ALL_REQUESTING);
		for (auto& remote : _hs.thickConsole_Arr) {
			auto mixIniStatus = remote.rawRegisters().get(R_SLAVE_REQUESTING_INITIALISATION);
			uint8_t requestINI_flag = MV_US_REQUESTING_INI << remote.index();
			status |= remote.sendSlaveIniData(requestINI_flag);
		}
		return status;
	}	
	
	uint8_t Initialiser::post_initialize_MixV() {
		uint8_t status = 0;
		for (auto& mixValve : hs().tempController().mixValveControllerArr) {
			uint8_t requestINI_flag = MV_US_REQUESTING_INI << mixValve.index();
			status |= mixValve.sendSlaveIniData(requestINI_flag);
		}
		return status;
	}

	uint8_t Initialiser::i2C_Test() {
		uint8_t status = _OK;
		status = _testDevices.speedTestDevices();
		_testDevices.testRelays();
		return status;
	}

	uint8_t Initialiser::postI2CResetInitialisation() {
		_resetI2C.hardReset.initialisationRequired = false;
		uint8_t status = relayPort().initialiseDevice();
		status |= post_initialize_Thick_Consoles();
		status |= post_initialize_MixV();
		if (status != _OK) logger() << L_time << F("  Initialiser::i2C_Test postI2CResetInitialisation failed") << L_endl;
		else logger() << L_time << F("  Initialiser::postI2CResetInitialisation OK\n");
		return status;
	}

	I_I2Cdevice_Recovery & Initialiser::getDevice(uint8_t deviceAddr) {
		if (deviceAddr == IO8_PORT_OptCoupl) return relayPort();
		else if (deviceAddr == MIX_VALVE_I2C_ADDR) return hs().tempController().mixValveControllerArr[0];
		else if (deviceAddr >= US_CONSOLE_I2C_ADDR && deviceAddr <= FL_CONSOLE_I2C_ADDR) {
			return hs().thickConsole_Arr[deviceAddr- US_CONSOLE_I2C_ADDR];
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

