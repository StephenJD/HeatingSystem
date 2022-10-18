#include "I2C_Reset.h"
#include <Logging.h>
#include <Logging_Loop.h>
#include <I2C_Recover.h>
#include <Watchdog_Timer.h>

#ifndef __AVR_ATmega328P__
namespace arduino_logger {
	Logger& loopLogger();
}
using namespace arduino_logger;
#endif
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;

namespace I2C_Recovery {

	/////////////////////////////////////////////////////
	//              I2C Reset Support                  //
	/////////////////////////////////////////////////////
#ifndef __AVR_ATmega328P__
	Pin_Wag HardReset::_i2c_resetPin{0,false,false };
	Pin_Wag HardReset::_arduino_resetPin{ 0,false,false };
	Pin_Wag HardReset::_led_indicatorPin{ 0,false,false };

	ResetI2C::ResetI2C(I2C_Recover_Retest & recover, I_IniFunctor & notify_reset_fn, I_TestDevices & testDevices, Pin_Wag i2c_resetPinWag, Pin_Wag arduino_resetPin, Pin_Wag led_indicatorPin)
		:
		_recover(&recover)
		, hardReset(i2c_resetPinWag, arduino_resetPin, led_indicatorPin)
		, _notify_reset_fn(&notify_reset_fn)
		, _testDevices(&testDevices)
		{
			_recover->setTimeoutFn(this);
			_recover->i2C().begin();
			_recover->i2C().setTimeouts(WORKING_SLAVE_BYTE_PROCESS_TIMOUT_uS, I2C_Talk::WORKING_STOP_TIMEOUT, 10000); // give generous stop-timeout in normal use 
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
		logger() << L_time << F("ResetI2C... for 0x") << L_hex << addr << L_flush;
		loopLogger() << F("ResetI2C for 0x") << L_hex << addr << L_endl;

		isInReset = true;
		Error_codes status = _OK;
		auto origFn = _recover->getTimeoutFn();
		_recover->setTimeoutFn(&hardReset);
		logger() << F("R_setTimeout_OK") << L_endl;

		hardReset(i2c, addr);
		notify_reset();
		loopLogger() << F("hardReset_OK") << L_endl;
		if (addr && !_recover->isRecovering()) {
			if (_testDevices == 0) {
				logger() << F("ResetI2C _testDevices is NULL") << L_flush;
				loopLogger() << F("_testDevices is NULL") << L_endl;
			} else {
				logger() << "\tResetI2C Doing retest" << L_flush;
				loopLogger() << F("R_getDevice...") << L_endl;
				I_I2Cdevice_Recovery& device = _testDevices->getDevice(addr);
				logger() << "\tR_testDevices..." << L_flush;
				loopLogger() << F("R_testDevices...") << L_endl;
				status = device.testDevice();
				logger() << "\tR_testDevices_OK" << L_flush;
				loopLogger() << F("R_testDevices_OK") << L_endl;
			}
			logger() << "\tR_postI2CReset-Req_OK" << L_flush;
			loopLogger() << F("R_postI2CReset-Req_OK") << L_endl;
		}

		_recover->setTimeoutFn(origFn);
		logger() << "\treset setTimeoutFn" << L_flush;
		loopLogger() << F("reset setTimeoutFn") << L_endl;
		isInReset = false;
		return status;
	}

	void ResetI2C::notify_reset() { 
		if (_notify_reset_fn == 0) {
			logger() << F("ResetI2C _notify_reset_fn is NULL") << L_flush;
			loopLogger() << F("_notify_reset_fn is NULL") << L_endl;
		} else {
			(*_notify_reset_fn)();
		}
	};
#endif

	unsigned long HardReset::_timeOfReset_uS = 1; // must be non-zero at startup

#ifndef __AVR_ATmega328P__
	void HardReset::arduinoReset(const char * msg) {
		logger() << L_time << F("\n *** HardReset::arduinoReset called by ") << msg << L_endl << L_flush;
		_arduino_resetPin.begin();
		_arduino_resetPin.set();
	}

	Error_codes HardReset::operator()(I2C_Talk & i2c, int addr) {
		_led_indicatorPin.set();
		_i2c_resetPin.set();
		delay_mS(128); // interrupts still serviced
		delay_mS(128); 
		delay_mS(128); 
		delay_mS(128); 
		_i2c_resetPin.clear();
		i2c.begin();
		_timeOfReset_uS = micros();
		if (_timeOfReset_uS == 0) _timeOfReset_uS = 1;
		logger() << L_time << F("Done Hard Reset for 0x") << L_hex << addr << L_flush;
		_led_indicatorPin.clear();
		if (digitalRead(20) == LOW)
			logger() << "\tData stuck after reset" << L_flush;
		return _OK;
	}
#endif

	bool HardReset::hasWarmedUp(bool wait) {
		if (_timeOfReset_uS != 0) {
			int32_t waitTime = _timeOfReset_uS + WARMUP_uS - micros();
			if (waitTime <= 0) _timeOfReset_uS = 0;
			else if (wait) {
				do {
#ifndef __AVR_ATmega328P__
					loopLogger() << L_time << "waitForWarmUp for mS " << waitTime/1000 << L_endl;
#endif
					reset_watchdog();
					delayMicroseconds(10000); // docs say delayMicroseconds cannot be relied upon > 16383uS.
					waitTime = _timeOfReset_uS + WARMUP_uS - micros();
				} while (waitTime > 0);
#ifndef __AVR_ATmega328P__
				loopLogger() << L_time << "waitForWarmUp_OK" << L_endl;
#endif
				_timeOfReset_uS = 0;
			}
			else {
#ifndef __AVR_ATmega328P__
				loopLogger() << L_time << "not WarmedUp..." << L_endl;
#endif
				return false;
			}
		}
		return true;
	}
}