#include "Initialiser.h"
#include "FactoryDefaults.h"
#include "..\HeatingSystem.h"
#include "..\HardwareInterfaces\Relay.h"
#include "..\HardwareInterfaces\I2C_Comms.h"
#include "..\HardwareInterfaces\A__Constants.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include <Logging/Logging.h>
#include <RDB.h>

using namespace client_data_structures;
using namespace RelationalDatabase;

namespace Assembly {
	using namespace RelationalDatabase;

	Initialiser::Initialiser(HeatingSystem & hs) 
		: _hs(hs),
		_resetI2C(_iniFunctor, _testDevices),
		_iniFunctor(*this),
		_testDevices(*this)
	{
		logger().log("  Initialiser PW check. req:", VERSION);
		if (!_hs.db.checkPW(VERSION)) {
			logger().log("  Initialiser PW Failed");
			setFactoryDefaults(_hs.db, VERSION);
		}
		auto dbFailed = false;
		for (auto & table : _hs.db) {
			if (!table.isOpen()) {
				logger().log("  Table not open at", (long)table.tableID());
				dbFailed = true;
				break;
			}
		}
		if (dbFailed) {
			logger().log("  dbFailed");
			setFactoryDefaults(_hs.db, VERSION);
		}
		logger().log("  Initialiser Constructed");
	}

	uint8_t Initialiser::i2C_Test() {
		auto err = _testDevices.speedTestDevices();
		/*if (!err) err = */_testDevices.testRelays();
		/*if (!err)*/ err = postI2CResetInitialisation();
		if (err != I2C_Helper::_OK) logger().log("  Initialiser::i2C_Test postI2CResetInitialisation failed");
		//else logger().log("  Initialiser::i2C_Test OK");
		return err;
	}

	uint8_t Initialiser::postI2CResetInitialisation() {
		return initialiseTempSensors()
			| _hs._tempController.relaysPort.initialiseDevice()
			| initialiseRemoteDisplays();
	}

	uint8_t Initialiser::initialiseTempSensors() {
		// Set room-sensors to high-res
		logger().log("Set room-sensors to high-res");
		return	_hs._tempController.tempSensorArr[T_DR].setHighRes()
			| _hs._tempController.tempSensorArr[T_FR].setHighRes()
			| _hs._tempController.tempSensorArr[T_UR].setHighRes();
	}

	uint8_t Initialiser::initialiseRemoteDisplays() {
		uint8_t failed = 0;
		for (auto & rd : _hs.remDispl) {
			failed |= rd.initialiseDevice();
		}
		logger().log("initialiseRemoteDisplays() done");
		return failed;
	}

	I2C_Helper::I_I2Cdevice & Initialiser::getDevice(uint8_t deviceAddr) {
		if (deviceAddr == IO8_PORT_OptCoupl) return hs()._tempController.relaysPort;
		else if (deviceAddr == MIX_VALVE_I2C_ADDR) return hs()._tempController.mixValveControllerArr[0];
		else if (deviceAddr >= 0x24 && deviceAddr <= 0x26) {
			return hs().remDispl[deviceAddr-0x24];
		}
		else {
			for (auto & ts : hs()._tempController.tempSensorArr) {
				if (ts.getAddress() == deviceAddr) return ts;
			}
			return hs()._tempController.tempSensorArr[0];
		}
	}

	uint8_t IniFunctor:: operator()() {
		return _ini->postI2CResetInitialisation();
	}
}

