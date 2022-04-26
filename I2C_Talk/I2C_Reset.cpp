#include "I2C_Reset.h"
#include <Logging.h>
#include <Logging_Loop.h>
#include <I2C_Recover.h>

namespace arduino_logger {
	Logger& loopLogger();
}
using namespace arduino_logger;
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;

namespace I2C_Recovery {

	/////////////////////////////////////////////////////
	//              I2C Reset Support                  //
	/////////////////////////////////////////////////////
	Pin_Wag HardReset::_i2c_resetPin{0,false,false };
	Pin_Wag HardReset::_arduino_resetPin{ 0,false,false };
	Pin_Wag HardReset::_led_indicatorPin{ 0,false,false };

	ResetI2C::ResetI2C(I2C_Recover_Retest & recover, I_IniFunctor & resetFn, I_TestDevices & testDevices, Pin_Wag i2c_resetPinWag, Pin_Wag arduino_resetPin, Pin_Wag led_indicatorPin)
		:
		_recover(&recover)
		, hardReset(i2c_resetPinWag, arduino_resetPin, led_indicatorPin)
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
			logger() << F("Recursive ResetI2C... for 0x") << L_hex << addr << L_flush;
			return _OK;
		}
		if (_recover == 0) {
			logger() << F("ResetI2C _recover is NULL for 0x") << L_hex << addr << L_flush;
			loopLogger() << F("ResetI2C _recover is NULL for 0x") << L_hex << addr << L_endl;
			return _OK;
		}
		logger() << F("\t\tResetI2C... for 0x") << L_hex << addr << L_flush;
		loopLogger() << F("ResetI2C for 0x") << L_hex << addr << L_endl;

		isInReset = true;
		Error_codes status = _OK;
		auto origFn = _recover->getTimeoutFn();
		_recover->setTimeoutFn(&hardReset);
		loopLogger() << F("setTimeout_OK") << L_endl;

		hardReset(i2c, addr);
		loopLogger() << F("hardReset_OK") << L_endl;
		if (addr && !_recover->isRecovering()) {
			if (_testDevices == 0) {
				logger() << F("ResetI2C _testDevices is NULL") << L_flush;
				loopLogger() << F("_testDevices is NULL") << L_endl;
			} else {
				logger() << "\tResetI2C Doing retest" << L_flush;
				loopLogger() << F("_getDevice...") << L_endl;
				I_I2Cdevice_Recovery& device = _testDevices->getDevice(addr);
				logger() << "\ttestDevices..." << L_flush;
				loopLogger() << F("_testDevices...") << L_endl;
				status = device.testDevice();
				logger() << "\ttestDevices_OK" << L_flush;
				loopLogger() << F("_testDevices_OK") << L_endl;
			}
			if (status == _OK) postResetInitialisation();
			logger() << "\tpostI2CResetInitialisation_OK" << L_flush;
			loopLogger() << F("_postI2CResetInitialisation_OK") << L_endl;
		}

		_recover->setTimeoutFn(origFn);
		logger() << "\treset setTimeoutFn" << L_flush;
		loopLogger() << F("reset setTimeoutFn") << L_endl;
		isInReset = false;
		return status;
	}

	void ResetI2C::postResetInitialisation() { 
		if (hardReset.initialisationRequired) {
			if (_postI2CResetInitialisation == 0) {
				logger() << F("ResetI2C _postI2CResetInitialisation is NULL") << L_flush;
				loopLogger() << F("_postI2CResetInitialisation is NULL") << L_endl;
			} else {
				logger() << F("\t\tResetI2C... _postI2CResetInitialisation") << L_flush;
				loopLogger() << F("_postI2CResetInitialisation...") << L_endl;
				if ((*_postI2CResetInitialisation)() != 0) return; // return 0 for OK. Resets hardReset.initialisationRequired to false.
			}
		}
	};

	unsigned long HardReset::_timeOfReset_mS = 1; // must be non-zero at startup

	void HardReset::arduinoReset(const char * msg) {
		logger() << L_time << F("\n *** HardReset::arduinoReset called by ") << msg << L_endl << L_flush;
		_arduino_resetPin.begin();
		_arduino_resetPin.set();
	}

	Error_codes HardReset::operator()(I2C_Talk & i2c, int addr) {
		_led_indicatorPin.set();
		_i2c_resetPin.set();
		delay(128); // interrupts still serviced
		delay(128); 
		delay(128); 
		delay(128); 
		_i2c_resetPin.clear();
		i2c.begin();
		_timeOfReset_mS = millis();
		if (_timeOfReset_mS == 0) _timeOfReset_mS = 1;
		logger() << L_time << F("Done Hard Reset for 0x") << L_hex << addr << L_flush;
		_led_indicatorPin.clear();
		if (digitalRead(20) == LOW)
			logger() << "\tData stuck after reset" << L_flush;
		initialisationRequired = true;
		return _OK;
	}

	void HardReset::waitForWarmUp() {
		if (_timeOfReset_mS != 0) {
			auto waitTime = _timeOfReset_mS + WARMUP_mS - millis();
			loopLogger() << L_time << "waitForWarmUp for mS " << waitTime << L_endl;
			delay(waitTime);
			//auto timeOut = Timer_mS(waitTime);
			//while (!timeOut) {
			//	loopLogger() << timeOut.timeLeft() << L_endl;
			//	delay(1000);
			//	loopLogger() << "del:1 " << timeOut.timeLeft() << L_endl;
			//}
			_timeOfReset_mS = 0;
		}
	}
	bool HardReset::hasWarmedUp() {
		if (_timeOfReset_mS != 0) {
			auto waitTime = _timeOfReset_mS + WARMUP_mS - millis();
			if (waitTime <= 0) _timeOfReset_mS = 0;
		}
		return _timeOfReset_mS == 0;
	}
}