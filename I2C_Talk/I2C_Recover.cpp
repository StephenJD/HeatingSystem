#include <I2C_Recover.h>
#include<I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>

#define DEBUG_TEST_DEVICE

#ifdef __AVR__
#undef DEBUG_TEST_DEVICE
#undef DEBUG_SPEED_TEST
#endif

#if defined DEBUG_SPEED_TEST || defined DEBUG_TEST_DEVICE
#include <Logging.h>
using namespace arduino_logger;
#endif

namespace I2C_Recovery {

	using namespace I2C_Talk_ErrorCodes;

	I2C_Recover::I2C_Recover(I2C_Talk& i2C) : _i2C(&i2C) { /*logger() << F("I2C_Recover") << L_endl;*/ /* Must not do i2C.begin() until entering setup()*/ }

	TwoWire & I2C_Recover::wirePort() { return i2C()._wire(); }

	Error_codes I2C_Recover::testDevice(int noOfTests, int allowableFailures) {
		auto status = _OK;
#ifdef DEBUG_TEST_DEVICE
		logger() << F("Test device 0x") << L_hex << device().getAddress() << L_endl;
#endif
		do {
			status = device().testDevice(); // called on I_I2Cdevice_Recovery device from I2C_Device.h. 
			// Calls non-recovering device-defined testDevice().
#ifdef DEBUG_TEST_DEVICE
			logger() << F("testDevice Tests/Failures ") << noOfTests << F("/") << allowableFailures << I2C_Talk::getStatusMsg(status) << L_endl;
#endif
			if (status != _OK) --allowableFailures;
			--noOfTests;
		} while (allowableFailures >= 0 && noOfTests > allowableFailures);
		return status;
	}

	Error_codes I2C_Recover::findAworkingSpeed() {
		constexpr int NO_OF_TESTS = 6;
		if (device().getStatus() == _disabledDevice) return _disabledDevice;
		auto addr = device().getAddress();
		constexpr int32_t tryFreq[] = { 52000,23000,78000,15000,118000,10000,177000,7000,266000,5000 };
		i2C().setI2CFrequency(35000); // Already tested at max speed
		auto testResult = i2C().status(addr);
#ifdef DEBUG_SPEED_TEST
		logger() << F("\nfindAworkingSpeed 0x") << L_hex << addr << F(" Exists ? At : ") << L_dec << i2C().getI2CFrequency() << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
		bool marginTimeout = false;
		if (testResult == _OK) {
			testResult = testDevice(NO_OF_TESTS, 1);
#ifdef DEBUG_SPEED_TEST
			logger() << F("\tfindAworkingSpeed Test at ") << i2C().getI2CFrequency() << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
		}
#ifndef DEBUG_TRY_ALL_SPEEDS
		if (testResult != _OK) {
#endif
			for (auto freq : tryFreq) {
				if (freq > i2C().max_i2cFreq()) continue;
				auto originalMargin = i2C().stopMargin();
				i2C().setI2CFrequency(freq);
				testResult = i2C().status(addr);
#ifdef DEBUG_SPEED_TEST				
				logger() << F("\tTried Exists for 0x") << L_hex << addr << F(" at freq: ") << L_dec << freq << F(" timeOut uS: ") << originalMargin << I2C_Talk::getStatusMsg(testResult) << L_endl;
#endif
				if (testResult == _OK) {
					device().set_runSpeed(freq);
					testResult = testDevice(NO_OF_TESTS, 1);
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