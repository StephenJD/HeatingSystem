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
		logger().log("Initialiser Started...");
		HardwareInterfaces::tempSensors = _hs.tempSensorArr;
		HardwareInterfaces::relays = _hs.relayArr;
		Relay::_relayRegister = &_hs.relaysPort._relayRegister;
		setFactoryDefaults(_hs.db);
		loadRelays();
		loadtempSensors();
		iniI2C();
		_hs.mixValveController.setResetTimePtr(&_resetI2C.hardReset.timeOfReset_mS);
		//_testDevices.speedTestDevices();
		//_testDevices.testRelays();
		postI2CResetInitialisation();
		logger().log("Initialiser Constructed");
	}

	void Initialiser::loadRelays() {
		auto q_relays = TableQuery(_hs.db.tableQuery(TB_Relay));
		int i = 0;
		for (Answer_R<R_Relay>thisRelay : q_relays) {
			relays[i].setPort(thisRelay->rec().port);
			++i;
		}
		logger().log("loadRelays Completed");
	}

	void Initialiser::loadtempSensors() {
		auto q_tempSensors = TableQuery(_hs.db.tableQuery(TB_TempSensor));
		int i = 0;
		for (Answer_R<R_TempSensor>thisTempSensor : q_tempSensors) {
			tempSensors[i].setAddress(thisTempSensor->rec().address);
			++i;
		}
		logger().log("loadtempSensors Completed");
	}

	void Initialiser::iniI2C() {
		_hs.relaysPort.setup(_hs.i2C, IO8_PORT_OptCoupl, ZERO_CROSS_PIN, RESET_OUT_PIN);
	}

	uint8_t Initialiser::postI2CResetInitialisation() {
		return initialiseTempSensors()
			| _hs.relaysPort.initialiseDevice()
			| initialiseRemoteDisplays();
	}

	uint8_t Initialiser::initialiseTempSensors() {
		// Set room-sensors to high-res
		logger().log("Set room-sensors to high-res");
		return	_hs.tempSensorArr[T_DR].setHighRes()
			| _hs.tempSensorArr[T_FR].setHighRes()
			| _hs.tempSensorArr[T_UR].setHighRes();
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
		if (deviceAddr == IO8_PORT_OptCoupl) return hs().relaysPort;
		else if (deviceAddr == MIX_VALVE_I2C_ADDR) return hs().mixValveController;
		else if (deviceAddr >= 0x24 && deviceAddr <= 0x26) {
			return hs().remDispl[deviceAddr-0x24];
		}
		else {
			for (auto & ts : hs().tempSensorArr) {
				if (ts.getAddress() == deviceAddr) return ts;
			}
			return hs().tempSensorArr[0];
		}
	}

	uint8_t IniFunctor:: operator()() {
		return _ini->postI2CResetInitialisation();
	}
}

