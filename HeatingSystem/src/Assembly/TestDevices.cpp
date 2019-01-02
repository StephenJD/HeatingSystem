#include "TestDevices.h"
#include "Initialiser.h"
#include "HeatingSystemEnums.h"
#include "..\HeatingSystem.h"
#include "..\HardwareInterfaces\A__Constants.h"
#include "..\HardwareInterfaces\Relay.h"
#include "..\HardwareInterfaces\Temp_Sensor.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include <Logging.h>
#include <RDB.h>

namespace HardwareInterfaces {
	using namespace RelationalDatabase;
	using namespace client_data_structures;
	using namespace Assembly;

	TestDevices::TestDevices(Initialiser & ini)
		: _ini(ini)
	{
		Serial.println("TestDevices Constructed");
	}

	HeatingSystem & TestDevices::hs() { return _ini.hs(); }

	int8_t TestDevices::showSpeedTestFailed(I2C_Helper::I_I2Cdevice & testFn, const char * device) {
		hs().mainDisplay.setCursor(5, 2);
		hs().mainDisplay.print("       0x");
		hs().mainDisplay.setCursor(5, 2);
		hs().mainDisplay.print(device);
		hs().mainDisplay.setCursor(14, 2);
		hs().mainDisplay.print(hs().i2C.result.foundDeviceAddr, HEX);
		hs().mainDisplay.print("    ");
		hs().mainDisplay.sendToDisplay();
		hs().i2C.speedTestS(&testFn);
		hs().mainDisplay.setCursor(17, 2);
		if (!hs().i2C.result.error == 0) {
			hs().mainDisplay.print("Bad");
			hs().mainDisplay.sendToDisplay();
			logger().log("TestDevices::speedTestDevices for ", hs().i2C.result.foundDeviceAddr, " Failed");
			delay(2000);
		}
		else {
			hs().mainDisplay.print(" OK");
			hs().mainDisplay.sendToDisplay();
			logger().log("TestDevices::speedTestDevices for ", hs().i2C.result.foundDeviceAddr, " OK at ", hs().i2C.result.thisHighestFreq);
		}
		logger().log();
		return hs().i2C.result.error;
	}

	I2C_Helper::I_I2Cdevice & TestDevices::getDevice(uint8_t deviceAddr) { return _ini.getDevice(deviceAddr); }

	uint8_t TestDevices::speedTestDevices(uint8_t deviceAddr) {// returns 0 for success or returns ERR_PORTS_FAILED / ERR_I2C_SPEED_FAIL
		enum { e_allPassed, e_mixValve, e_relays = 2, e_US_Remote = 4, e_FL_Remote = 8, e_DS_Remote = 16, e_TempSensors = 32 };
		hs().i2C.setTimeoutFn(&_ini._resetI2C.hardReset);
		int8_t returnVal = 0;
		hs().mainDisplay.setCursor(0, 2);
		hs().mainDisplay.print("Test ");
		logger().log("\nTestDevices::speedTestDevices has been called");
		hs().i2C.result.reset();

		logger().log("TestDevices::speedTestDevices\tTry Relay Port");
		hs().i2C.result.foundDeviceAddr = hs().relaysPort.getAddress();
		if (showSpeedTestFailed(hs().relaysPort, "Relay")) {
			//f->eventS().newEvent(ERR_PORTS_FAILED, 0);
			returnVal = ERR_PORTS_FAILED;
		}

		logger().log("TestDevices::speedTestDevices\tTry Remotes");
		hs().i2C.result.foundDeviceAddr = DS_REMOTE_ADDRESS;
		if (showSpeedTestFailed(hs().remDispl[D_Hall], "DS Rem")) {
			//f->eventS().newEvent(ERR_I2C_READ_FAILED, 0);
			returnVal = ERR_I2C_READ_FAILED;
		}

		hs().i2C.result.foundDeviceAddr = US_REMOTE_ADDRESS;
		if (showSpeedTestFailed(hs().remDispl[D_Bedroom], "US Rem")) {
			//f->eventS().newEvent(ERR_I2C_READ_FAILED, 0);
			returnVal = ERR_I2C_READ_FAILED;
		}

		hs().i2C.result.foundDeviceAddr = FL_REMOTE_ADDRESS;
		if (showSpeedTestFailed(hs().remDispl[D_Flat], "FL Rem")) {
			//f->eventS().newEvent(ERR_I2C_READ_FAILED, 0);
			returnVal = ERR_I2C_READ_FAILED;
		}

		logger().log("TestDevices::speedTestDevices\tTry Mix Valve");
		hs().i2C.result.foundDeviceAddr = MIX_VALVE_I2C_ADDR;
		if (showSpeedTestFailed(hs().mixValveController, "Mix V")) {
		//	//f->eventS().newEvent(ERR_MIX_ARD_FAILED, 0);
			returnVal = ERR_MIX_ARD_FAILED;
		}
		
		for (auto & ts : hs().tempSensorArr) {
			logger().log("TestDevices::speedTestDevices\tTry TS", ts.getAddress());
			hs().i2C.result.foundDeviceAddr = ts.getAddress();
			showSpeedTestFailed(ts, "TS");
		}

		hs().mainDisplay.setCursor(12, 2);
		hs().mainDisplay.print(hs().i2C.result.maxSafeSpeed);
		hs().mainDisplay.sendToDisplay();
		logger().log("I2C Max Speed is ", hs().i2C.result.maxSafeSpeed);
		hs().i2C.setTimeoutFn(&_ini._resetI2C);
		hs().i2C.result.reset();
		return returnVal;
	}

	uint8_t TestDevices::testRelays() {
		/////////////////////// Cycle through ports /////////////////////////
		uint8_t returnVal = 0;
		uint8_t numberFailed = 0;
		logger().log("Relay_Run::testRelays");
		if (hs().relaysPort.testDevice(hs().i2C, IO8_PORT_OptCoupl)) {
			//f->eventS().newEvent(ERR_PORTS_FAILED, 0);
			logger().log("Relay_Run::testRelays\tNo Relays\tERR_PORTS_FAILED");
			return NO_OF_RELAYS;
		}
		hs().mainDisplay.setCursor(0, 3);
		hs().mainDisplay.print("Relay Test: ");
		for (int relayNo = 0; relayNo < NO_OF_RELAYS; ++relayNo) { // 12 relays, but 1 is mix enable.
			hs().relayArr[RELAY_ORDER[relayNo]].setRelay(1);
			returnVal = hs().relaysPort.setAndTestRegister();
			delay(200);
			hs().relayArr[RELAY_ORDER[relayNo]].setRelay(0);
			returnVal |= hs().relaysPort.setAndTestRegister();
			numberFailed = numberFailed + (returnVal != 0);
			hs().mainDisplay.setCursor(12, 3);
			hs().mainDisplay.print(relayNo, DEC);
			if (returnVal) {
				//f->eventS().newEvent(ERR_PORTS_FAILED, relayNo);
				logger().log("Relay_Run::testRelays\tRelay Failed\tERR_PORTS_FAILED", relayNo);
				hs().mainDisplay.print(" Fail");
			}
			else {
				hs().mainDisplay.print(" OK  ");
			}
			hs().mainDisplay.sendToDisplay();
			delay(200);
			returnVal = 0;
		}
		hs().mainDisplay.setCursor(12, 3);
		if (numberFailed > 0) {
			logger().log("Failed :",numberFailed);
			hs().mainDisplay.print(numberFailed);
			hs().mainDisplay.print("Failed");
		}
		else {
			logger().log("All OK");
			hs().mainDisplay.print("All OK  ");
			//I2C_OK = true;
		}
		hs().mainDisplay.sendToDisplay();
		delay(500);
		return numberFailed;
	}

}