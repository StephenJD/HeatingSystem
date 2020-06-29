#include <I2C_Recover.h>
#include<I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>

#ifdef DEBUG_SPEED_TEST
#include <Logging.h>
#endif

namespace I2C_Recovery {

	using namespace I2C_Talk_ErrorCodes;

	TwoWire & I2C_Recover::wirePort() { return i2C()._wire(); }

	void I2C_Recover::wireBegin() { 
		//Serial.print("I2C_Recover::wire: "); Serial.println(long(&i2C())); Serial.flush();
		i2C().wireBegin();
	}

	error_codes I2C_Recover::testDevice(int, int) {
		return device().testDevice();
	}

	error_codes I2C_Recover::findAworkingSpeed() {
		if (device().getStatus() == _disabledDevice) return _disabledDevice;
		auto addr = device().getAddress();
		// Must test MAX_I2C_FREQ as this is skipped in later tests
		constexpr uint32_t tryFreq[] = { 52000,8200,330000,3200,21000,2000,5100,13000,33000,83000,210000 };
		i2C().setI2CFrequency(i2C().max_i2cFreq());
		//logger() << F("findAworkingSpeed Try Exists? At:"), i2C().getI2CFrequency());
		auto testResult = i2C().status(addr);
		bool marginTimeout = false;
#ifdef DEBUG_SPEED_TEST
		logger() << F("findAworkingSpeed Exists? At:") << i2C().getI2CFrequency() << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
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
				i2C().setI2CFrequency(freq);
				testResult = i2C().status(addr);
#ifdef DEBUG_SPEED_TEST				
				logger() << F("\tTried Exists for 0x") << L_hex << addr << F(" at freq: ") << L_dec << freq << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
				if (testResult == _OK) {
					device().set_runSpeed(freq);
					testResult = testDevice(2, 1);
					if (testResult == _StopMarginTimeout) marginTimeout = true;
#ifdef DEBUG_SPEED_TEST					
					logger() << F("\tfindAworkingSpeed Test at ") << i2C().getI2CFrequency() << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
#ifndef DEBUG_TRY_ALL_SPEEDS					
					if (testResult == _OK) break;
#endif
				}
			}
#ifndef DEBUG_TRY_ALL_SPEEDS
		}
#else
		testResult = _OK;
#endif
		if (marginTimeout) testResult = _StopMarginTimeout;
		else if (testResult != _OK) testResult = _I2C_Device_Not_Found;
		return testResult;
	}
}