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
	using namespace I2C_Talk_ErrorCodes;

	TestDevices::TestDevices(Initialiser & ini)
		: _ini(ini)
	{
		logger() << "TestDevices Constructed" << L_endl;
	}

	HeatingSystem & TestDevices::hs() { return _ini.hs(); }

	int8_t TestDevices::showSpeedTestFailed(I_I2Cdevice_Recovery & device, const char * deviceName) {
		hs().mainDisplay.setCursor(5, 2);
		hs().mainDisplay.print("       0x");
		hs().mainDisplay.setCursor(5, 2);
		hs().mainDisplay.print(deviceName);
		hs().mainDisplay.setCursor(14, 2);
		hs().mainDisplay.print(device.getAddress(), HEX);
		hs().mainDisplay.print("    ");
		hs().mainDisplay.sendToDisplay();
		auto speedTest = I2C_SpeedTest{ device };
		speedTest.fastest();
		hs().mainDisplay.setCursor(17, 2);
		if (speedTest.error() != _OK) {
			hs().mainDisplay.print("Bad");
			hs().mainDisplay.sendToDisplay();
			 logger() << "TestDevices::speedTestDevices for " << device.getAddress() << " Failed" << L_endl;
#ifndef ZPSIM
			delay(2000);
#endif
		}
		else {
			hs().mainDisplay.print(" OK");
			hs().mainDisplay.sendToDisplay();
			 logger() << "TestDevices::speedTestDevices for " << device.getAddress() << " OK at " << speedTest.thisHighestFreq() << L_endl;
		}
		logger() << L_endl;
		return speedTest.error();
	}

	I_I2Cdevice_Recovery & TestDevices::getDevice(uint8_t deviceAddr) { return _ini.getDevice(deviceAddr); }

	uint8_t TestDevices::speedTestDevices() {// returns 0 for success or returns ERR_PORTS_FAILED / ERR_I2C_SPEED_FAIL
		enum { e_allPassed, e_mixValve, e_relays = 2, e_US_Remote = 4, e_FL_Remote = 8, e_DS_Remote = 16, e_TempSensors = 32 };
		hs()._recover.setTimeoutFn(&_ini._resetI2C.hardReset);
		int8_t returnVal = 0;
		hs().mainDisplay.setCursor(0, 2);
		hs().mainDisplay.print("Test ");
		 logger() << "\n\nTestDevices::speedTestDevices has been called";
		 logger() << "\nTestDevices::speedTestDevices\tTry Relay Port" << L_endl;
		if (showSpeedTestFailed(hs()._tempController.relaysPort, "Relay")) {
			returnVal = ERR_PORTS_FAILED;
		}

		 logger() << "TestDevices::speedTestDevices\tTry Remotes" << L_endl;
		if (showSpeedTestFailed(hs().remDispl[D_Hall], "DS Rem")) {
			returnVal = ERR_I2C_READ_FAILED;
		}

		if (showSpeedTestFailed(hs().remDispl[D_Bedroom], "US Rem")) {
			returnVal = ERR_I2C_READ_FAILED;
		}

		if (showSpeedTestFailed(hs().remDispl[D_Flat], "FL Rem")) {
			returnVal = ERR_I2C_READ_FAILED;
		}

		 logger() << "TestDevices::speedTestDevices\tTry Mix Valve" << L_endl;
		if (showSpeedTestFailed(hs()._tempController.mixValveControllerArr[0], "Mix V")) {
			returnVal = ERR_MIX_ARD_FAILED;
		}
		
		for (auto & ts : hs()._tempController.tempSensorArr) {
			 logger() << "TestDevices::speedTestDevices\tTry TS " << ts.getAddress() << L_endl;
			showSpeedTestFailed(ts, "TS");
		}

		hs().mainDisplay.sendToDisplay();
		hs()._recover.setTimeoutFn(&_ini._resetI2C);
		logger() << L_endl;
		return returnVal;
	}

	uint8_t TestDevices::testRelays() {
		//******************** Cycle through ports *************************
		uint8_t returnVal = 0;
		uint8_t numberFailed = 0;
		 logger() << "Relay_Run::testRelays";
		if (hs()._tempController.relaysPort.testDevice()) {
			 logger() << "\nRelay_Run::testRelays\tNo Relays\tERR_PORTS_FAILED";
			return NO_OF_RELAYS;
		}
		hs().mainDisplay.setCursor(0, 3);
		hs().mainDisplay.print("Relay Test: ");
		for (int relayNo = 0; relayNo < NO_OF_RELAYS; ++relayNo) { // 12 relays, but 1 is mix enable.
			hs()._tempController.relayArr[RELAY_ORDER[relayNo]].setRelay(1);
			returnVal = hs()._tempController.relaysPort.setAndTestRegister();
			delay(200);
			hs()._tempController.relayArr[RELAY_ORDER[relayNo]].setRelay(0);
			returnVal |= hs()._tempController.relaysPort.setAndTestRegister();
			numberFailed = numberFailed + (returnVal != 0);
			hs().mainDisplay.setCursor(12, 3);
			hs().mainDisplay.print(relayNo, DEC);
			if (returnVal) {
				 logger() << "\nRelay_Run::testRelays\tRelay Failed\tERR_PORTS_FAILED " << relayNo;
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
			 logger() << "\nFailed : " << numberFailed << L_endl;
			hs().mainDisplay.print(numberFailed);
			hs().mainDisplay.print("Failed");
		}
		else {
			 logger() << "\nAll OK\n" << L_endl;
			hs().mainDisplay.print("All OK  ");
			//I2C_OK = true;
		}
		hs().mainDisplay.sendToDisplay();
		delay(500);
		return numberFailed;
	}

}