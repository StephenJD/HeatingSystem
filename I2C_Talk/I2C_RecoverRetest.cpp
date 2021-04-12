#include "I2C_RecoverRetest.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>
#include <Logging.h>

void ui_yield();

namespace I2C_Recovery {

	using namespace I2C_Talk_ErrorCodes;
	int I2C_Recover_Retest::_deviceWaitingOnFailureFor10Mins;

	Error_codes I2C_Recover_Retest::testDevice(int noOfTests, int allowableFailures) { // Tests should all be non-recovering
		auto status = _OK;
		//logger() << F("Test device 0x") << L_hex << device().getAddress() << F(" at ") << 
		do {
			status = device().testDevice(); // called on I_I2Cdevice_Recovery device from I2C_Device.h. 
			// Calls non-recovering device-defined testDevice().
			
			if (status != _OK) {
				if (status == _BusReleaseTimeout) {
					call_timeOutFn(device().getAddress());
				}
				--allowableFailures;
			}
			//logger() << F("testDevice Tests/Failures ") << noOfTests << F_SLASH << allowableFailures << I2C_Talk::getStatusMsg(status) << L_endl;
			--noOfTests;
		} while (allowableFailures >= 0 && noOfTests > allowableFailures);
		return status;
	}

	void I2C_Recover_Retest::setTimeoutFn(I_I2CresetFunctor * timeOutFn) { _timeoutFunctor = timeOutFn; } // Timeout function pointer

	void I2C_Recover_Retest::setTimeoutFn(TestFnPtr timeOutFn) { // convert function into functor
		_i2CresetFunctor.set(timeOutFn);
		_timeoutFunctor = &_i2CresetFunctor;
	} // Timeout function pointer

	Error_codes I2C_Recover_Retest::newReadWrite(I_I2Cdevice_Recovery & i2Cdevice, int retries) {
		if (_isRecovering) return i2Cdevice.isEnabled() ? _OK : _disabledDevice;
		//logger() << F("newReadWrite Register device 0x") << L_hex << device().getAddress() << L_endl;
		if (i2Cdevice.reEnable() == _disabledDevice) {
			return _disabledDevice;
		}
		registerDevice(i2Cdevice);
		_retries = retries;
		//logger() << F("newReadWrite setspeed to: ") << device.runSpeed() << L_endl;
		i2C().setI2CFrequency(i2Cdevice.runSpeed());
		return _OK;
	}

	void I2C_Recover_Retest::endReadWrite() {
		if (_isRecovering) return;
		if (_strategy.strategy() != S_NoProblem) _strategy.succeeded();
	}

	void I2C_Recover_Retest::basicTestsBeforeScan(I_I2Cdevice_Recovery & dev) {
		registerDevice(dev);
	}

	bool I2C_Recover_Retest::tryReadWriteAgain(Error_codes status) {
		static uint32_t recoveryTime;
		static Strategy maxStrategyUsed = S_NoProblem;

		// Lambdas
		auto recoveryWasAttempted = []() { return recoveryTime > 0; };
		auto getFinalStrategyRecorded = [this]() {strategy().tryAgain(maxStrategyUsed);};
		auto resetRecoveryStrategy = []() {maxStrategyUsed = S_NoProblem; recoveryTime = 0;};
		auto isRecursiveCall = [this]() { return _isRecovering; };
		auto haveBumpedUpMaxStrategyUsed = [](Strategy thisStrategy) {if (maxStrategyUsed < thisStrategy) { maxStrategyUsed = thisStrategy; return true; } else return false; };

		uint32_t strategyStartTime;
		bool shouldTryReadingAgain = false;
		if (status == _OK) {
			if (recoveryWasAttempted()) {
				if (I_I2Cdevice_Recovery::I2C_RETRIES - _retries > 2) logger() << L_time << L_tabs << F("Max Strategy") << maxStrategyUsed << I_I2Cdevice_Recovery::I2C_RETRIES - _retries << recoveryTime
					 << device().runSpeed() << L_hex << device().getAddress() << L_endl;
				auto thisMaxStrategy = maxStrategyUsed;
				(*_timeoutFunctor).postResetInitialisation();
				maxStrategyUsed = thisMaxStrategy;
				getFinalStrategyRecorded();
				resetRecoveryStrategy();
			}
			endReadWrite();
			recoveryTime = 0;
			if (device().getAddress() == abs(_deviceWaitingOnFailureFor10Mins)) _deviceWaitingOnFailureFor10Mins = 0;
		} else if (_retries > 0) {
			--_retries;
			strategyStartTime = micros();
			restart("");
			recoveryTime += micros() - strategyStartTime;
			return true;
		} else {
			shouldTryReadingAgain = true;
			_isRecovering = true;
			logger() << L_time << F("tryReadWriteAgain: device 0x") << L_hex << device().getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << device().runSpeed() << L_endl;
			_strategy.next();

			switch (_strategy.strategy()) {
			case S_TryAgain: // 1
			case S_SlowDown: // 2
				haveBumpedUpMaxStrategyUsed(S_SlowDown);
				logger() << F("\t\tS_Slow-down") << L_endl;
				strategyStartTime = micros();
				{
					//auto slowedDown = slowdown();
					recoveryTime = micros() - strategyStartTime;
					//if (slowedDown) {
					//	strategy().tryAgain(S_TryAgain); // keep slowing down until can't.
					//	break;
					//} else {
						logger() << F("\n    Slow-down did nothing...\n");
						if (device().testDevice() == _OK) break;
					//}
				}
			case S_SpeedTest: // 3
				if (haveBumpedUpMaxStrategyUsed(S_SpeedTest)) {
					logger() << F("\t\tS_SpeedTest") << L_endl;
					{ // code block
						strategyStartTime = micros();
						auto speedTest = I2C_SpeedTest(device());
						speedTest.fastest();
						recoveryTime = micros() - strategyStartTime;
						if (speedTest.error() == _OK) {
							strategy().tryAgain(S_TryAgain);
							logger() << F("\t\tNew Speed ") << device().runSpeed() << L_endl;
						}
						else {
							logger() << F("\t\tDevice Failed\n");
							disable();
						}
						device().i2C().setStopMargin(I2C_Talk::WORKING_STOP_TIMEOUT);
						if (device().testDevice() == _OK) break;
					} // fall-through on error
				} //else logger() << F("\t\tTry again S_SpeedTest 0x");
			case S_PowerDown: // 4
				if (haveBumpedUpMaxStrategyUsed(S_PowerDown)) {
					//logger() << F("\t\tS_Power-Down") << L_endl;
					if (status == _BusReleaseTimeout) {
						strategyStartTime = micros();
						call_timeOutFn(device().getAddress());
					}
					//strategy().tryAgain(S_SlowDown); // when incremented will do a speed-test
					if (device().testDevice() == _OK) break;
				} // fall-through
			case S_Disable: // 5
				if (haveBumpedUpMaxStrategyUsed(S_Disable)) {
					logger() << L_time << F("S_Disable device 0x") << L_hex << device().getAddress() << L_endl;
				}			
				if (device().getAddress() == _deviceWaitingOnFailureFor10Mins) { // waiting 10 mins didn't fix it, so do full machine reset
					logger() << L_time << F("S_DeviceUnrecoverable device 0x") << L_hex << device().getAddress() << L_endl;
					haveBumpedUpMaxStrategyUsed(S_Unrecoverable);
					_deviceWaitingOnFailureFor10Mins = -_deviceWaitingOnFailureFor10Mins; // signals failed on second attempt
				} else {
					if (_deviceWaitingOnFailureFor10Mins == 0) {
						logger() << L_time << F("New Disabled device 0x") << L_hex << device().getAddress() << L_endl;
						_deviceWaitingOnFailureFor10Mins = device().getAddress();
					}
				} // fall-through
			case S_Unrecoverable: // 6
				disable();
				getFinalStrategyRecorded();
				resetRecoveryStrategy();
				_isRecovering = false;				
				endReadWrite();
				shouldTryReadingAgain = false;
				break;
			default:;
			}
			_isRecovering = false;
		}
		ui_yield();
		return shouldTryReadingAgain;
	}

	bool I2C_Recover_Retest::restart(const char * name) {
		i2C().begin();
		return true;
	}

	bool I2C_Recover_Retest::slowdown() { // called by failure strategy
		bool canReduce = false;
		//if (millis() - device().getFailedTime() < REPEAT_FAILURE_PERIOD) { // within 10secs try reducing speed.
			auto thisFreq = device().runSpeed();
			canReduce = thisFreq > I2C_Talk::MIN_I2C_FREQ;
			if (canReduce) {
				logger() << F("\t\tslowdown device 0x") << L_hex << device().getAddress() << F(" speed was ") << L_dec << thisFreq << L_endl;
				device().set_runSpeed(max(thisFreq - max(thisFreq / 10, 1000), I2C_Talk::MIN_I2C_FREQ));
				logger() << F("\t\treduced speed to ") << device().runSpeed() << L_endl;
			}
		//}
		return canReduce;
	}

	void I2C_Recover_Retest::call_timeOutFn(int addr) {
		static bool doingTimeOut;
		if (_timeoutFunctor != 0) {
			if (doingTimeOut) {
				logger() << F("\n\t**** call_timeOutFn called recursively ***") << L_endl;
				return;
			}
			doingTimeOut = true;
			//logger() << F("\ncall_timeOutFn") << L_endl;
			(*_timeoutFunctor)(i2C(), device().getAddress());
		}
		else {
			logger() << F("\n\t***  no Time_OutFn set ***") << L_endl;
			i2C().begin(); // restart i2c in case this is called after an i2c failure
		}
		doingTimeOut = false;
	}
}