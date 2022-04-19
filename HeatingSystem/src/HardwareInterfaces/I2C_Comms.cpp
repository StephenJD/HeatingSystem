#include "I2C_Comms.h"
#include <Logging.h>
#include "A__Constants.h"
#include <I2C_Recover.h>
#include <Mix_Valve.h>
#include <OLED_Thick_Display.h>

using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;
using namespace arduino_logger;

namespace HardwareInterfaces {

	void wait_DevicesToFinish(i2c_registers::RegAccess reg) {
		if (reg.get(R_PROG_STATE)) {
			auto timeout = Timer_mS(100);
			while (!timeout && reg.get(R_PROG_STATE)) {}
			//auto delayedBy = timeout.timeUsed();
			//logger() << L_time << "WaitedforI2C: " << delayedBy << L_endl;
			reg.set(R_PROG_STATE, 0);
		}
	};

	/////////////////////////////////////////////////////
	//              I2C Reset Support                  //
	/////////////////////////////////////////////////////

	ResetI2C::ResetI2C(I2C_Recover_Retest & recover, I_IniFunctor & resetFn, I_TestDevices & testDevices, Pin_Wag resetPinWag)
		:
		_recover(&recover)
		, hardReset(resetPinWag)
		, _postI2CResetInitialisation(&resetFn)
		, _testDevices(&testDevices)
		{
			_recover->setTimeoutFn(this);
			_recover->i2C().begin();
			_recover->i2C().setTimeouts(15000, I2C_Talk::WORKING_STOP_TIMEOUT, 10000); // give generous stop-timeout in normal use 
		}

	Error_codes ResetI2C::operator()(I2C_Talk & i2c, int addr) {
		static bool isInReset = false;
		if (isInReset) {
			logger() << F("\nTest: Recursive Reset... for 0x") << L_hex << addr << L_endl;
			return _OK;
		}
		if (_recover == 0) {
			logger() << F("\nReset _recover is NULL for 0x") << L_hex << addr << L_endl;
			return _OK;
		}

		isInReset = true;
		Error_codes status = _OK;
		auto origFn = _recover->getTimeoutFn();
		_recover->setTimeoutFn(&hardReset);

		logger() << F("\t\tResetI2C... for 0x") << L_hex << addr << L_endl;
		hardReset(i2c, addr);
		if (!_recover->isRecovering()) {
			logger() << "\tResetI2C Doing retest" << L_endl;
			I_I2Cdevice_Recovery & device = _testDevices->getDevice(addr);
			status = device.testDevice();
			if (status == _OK) postResetInitialisation();
		}

		_recover->setTimeoutFn(origFn);
		isInReset = false;
		return status;
	}

	void ResetI2C::postResetInitialisation() { 
		if (hardReset.initialisationRequired && _postI2CResetInitialisation) {
			logger() << F("\t\tResetI2C... _postI2CResetInitialisation\n");
			if ((*_postI2CResetInitialisation)() != 0) return; // return 0 for OK. Resets hardReset.initialisationRequired to false.
		}
	};

	void HardReset::arduinoReset(const char * msg) {
		logger() << L_time << F("\n *** HardReset::arduinoReset called by ") << msg << L_endl << L_flush;
		pinMode(RESET_5vREF_PIN, OUTPUT);
		digitalWrite(RESET_5vREF_PIN, LOW);
	}

	Error_codes HardReset::operator()(I2C_Talk & i2c, int addr) {
		LocalKeypad::indicatorLED().set();
		_resetPinWag.set();
		delay(128); // interrupts still serviced
		delay(128); 
		delay(128); 
		delay(128); 
		_resetPinWag.clear();
		i2c.begin();
		timeOfReset_mS = millis();
		if (timeOfReset_mS == 0) timeOfReset_mS = 1;
		logger() << L_time << F("Done Hard Reset for 0x") << L_hex << addr << L_endl;
		LocalKeypad::indicatorLED().clear();
		if (digitalRead(20) == LOW)
			logger() << "\tData stuck after reset" << L_endl;
		initialisationRequired = true;
		return _OK;
	}

}