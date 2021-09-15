#include <I2C_Recover.h>
#include<I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>

#ifdef DEBUG_SPEED_TEST
#include <Logging.h>
using namespace arduino_logger;
#endif

void ui_yield();

namespace I2C_Recovery {

	using namespace I2C_Talk_ErrorCodes;

	I2C_Recover::I2C_Recover(I2C_Talk& i2C) : _i2C(&i2C) { /*logger() << F("I2C_Recover") << L_endl;*/ /* Must not do i2C.begin() until entering setup()*/ }

	TwoWire & I2C_Recover::wirePort() { return i2C()._wire(); }

	Error_codes I2C_Recover::testDevice(int, int) {
		return device().testDevice();
	}

	Error_codes I2C_Recover::findAworkingSpeed() {
		if (device().getStatus() == _disabledDevice) return _disabledDevice;
		auto addr = device().getAddress();
		// Must test MAX_I2C_FREQ as this is skipped in later tests
		constexpr uint32_t tryFreq[] = { 52000,8200,330000,3200,21000,2000,5100,13000,33000,83000,210000 };
		i2C().setI2CFrequency(i2C().max_i2cFreq());
		auto testResult = i2C().status(addr);
#ifdef DEBUG_SPEED_TEST
		logger() << F("findAworkingSpeed Exists? At:") << i2C().getI2CFrequency() << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
		bool marginTimeout = false;
		if (testResult == _OK) {
			testResult = testDevice(2, 1);
#ifdef DEBUG_SPEED_TEST
			logger() << F("\tfindAworkingSpeed Test at ") << i2C().getI2CFrequency() << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
		}
#ifndef DEBUG_TRY_ALL_SPEEDS
		if (testResult != _OK) {
#endif
			for (auto freq : tryFreq) {
				auto originalMargin = i2C().stopMargin();
				i2C().setI2CFrequency(freq);
				testResult = i2C().status(addr);
#ifdef DEBUG_SPEED_TEST				
				logger() << F("\tTried Exists for 0x") << L_hex << addr << F(" at freq: ") << L_dec << freq << F(" timeOut uS: ") << originalMargin << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
				if (testResult == _OK) {
					device().set_runSpeed(freq);
					testResult = testDevice(2, 1);
					if (testResult == _StopMarginTimeout) {
						marginTimeout = true;
						break;
					}
#ifdef DEBUG_SPEED_TEST					
					logger() << F("\tfindAworkingSpeed Test at ") << i2C().getI2CFrequency() << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
#ifndef DEBUG_TRY_ALL_SPEEDS					
					if (testResult == _OK) break;
#endif
				}
				i2C().setStopMargin(I2C_Talk::WORKING_STOP_TIMEOUT);
				ui_yield();
				i2C().setStopMargin(originalMargin);
			}
#ifdef DEBUG_TRY_ALL_SPEEDS
		testResult = _OK;
#else
		}
#endif
		if (marginTimeout) testResult = _StopMarginTimeout;
		else if (testResult != _OK) testResult = _I2C_Device_Not_Found;
		return testResult;
	}
}