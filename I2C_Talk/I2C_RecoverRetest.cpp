#include "I2C_RecoverRetest.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>

#if defined DEBUG_RECOVER || defined DEBUG_SPEED_TEST || defined REPORT_RECOVER
#include <Logging.h>
using namespace arduino_logger;
#endif

namespace I2C_Recovery {

	using namespace I2C_Talk_ErrorCodes;
	int I2C_Recover_Retest::_deviceWaitingOnFailureFor10Mins;

	Error_codes I2C_Recover_Retest::testDevice(int noOfTests, int allowableFailures) { // Tests should all be non-recovering
		auto status = _OK;
#ifdef DEBUG_SPEED_TEST
		logger() << F("Test device 0x") << L_hex << device().getAddress() << L_endl;
#endif
		do {
			status = device().testDevice(); // called on I_I2Cdevice_Recovery device from I2C_Device.h. 
			// Calls non-recovering device-defined testDevice().
			
			if (status != _OK) {
				if (status == _BusReleaseTimeout) {
					call_timeOutFn(device().getAddress());
				}
				--allowableFailures;
			}
#ifdef DEBUG_SPEED_TEST
			logger() << F("testDevice Tests/Failures ") << noOfTests << F("/") << allowableFailures << I2C_Talk::getStatusMsg(status) << L_endl;
#endif
			--noOfTests;
			//delayMicroseconds(2000);
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
#ifdef DEBUG_RECOVER
		logger() << F("newReadWrite Register device 0x") << L_hex << i2Cdevice.getAddress() << L_endl;
#endif
		if (i2Cdevice.reEnable() == _disabledDevice) {
			return _disabledDevice;
		}
		registerDevice(i2Cdevice);
		_retries = retries;
#ifdef DEBUG_RECOVER
		logger() << F("newReadWrite setspeed to: ") << i2Cdevice.runSpeed() << L_endl;
#endif
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
#ifdef DEBUG_RECOVER
			logger() << L_time << F("tryReadWriteAgain: device 0x") << L_hex << device().getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << device().runSpeed() << L_endl;
#endif
			if (recoveryWasAttempted()) {
#ifdef DEBUG_RECOVER
				if (I_I2Cdevice_Recovery::I2C_RETRIES - _retries > 2) logger() << L_time << L_tabs << F("Max Strategy") << maxStrategyUsed << I_I2Cdevice_Recovery::I2C_RETRIES - _retries << recoveryTime
					 << device().runSpeed() << L_hex << device().getAddress() << L_flush;
#endif
				auto thisMaxStrategy = maxStrategyUsed;
				if (_timeoutFunctor) (*_timeoutFunctor).postResetInitialisation();
				maxStrategyUsed = thisMaxStrategy;
				getFinalStrategyRecorded();
				resetRecoveryStrategy();
			}
			endReadWrite();
			recoveryTime = 0;
			if (device().getAddress() == abs(_deviceWaitingOnFailureFor10Mins)) _deviceWaitingOnFailureFor10Mins = 0;
		} else if (_retries > 0) {
#ifdef DEBUG_RECOVER
			logger() << L_time << F("tryReadWriteAgain: device 0x") << L_hex << device().getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << device().runSpeed() << L_endl;
#endif
			--_retries;
			strategyStartTime = micros();
			restart("");
			recoveryTime += micros() - strategyStartTime;
			return true;
		} else {
			shouldTryReadingAgain = true;
			_isRecovering = true;
#ifdef REPORT_RECOVER
			logger() << L_time << F("tryReadWriteAgain: device 0x") << L_hex << device().getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << device().runSpeed() << L_endl;
#endif
			_strategy.next();

			switch (_strategy.strategy()) {
			case S_TryAgain: // 1
			case S_SlowDown: // 2
				haveBumpedUpMaxStrategyUsed(S_SlowDown);
#ifdef REPORT_RECOVER
				logger() << F("\t\tS_Slow-down") << L_endl;
#endif
				strategyStartTime = micros();
				{
					//auto slowedDown = slowdown();
					recoveryTime = micros() - strategyStartTime;
					//if (slowedDown) {
					//	strategy().tryAgain(S_TryAgain); // keep slowing down until can't.
					//	break;
					//} else {
#ifdef REPORT_RECOVER
						logger() << F("\n    Slow-down did nothing...\n");
#endif
						if (device().testDevice() == _OK) break;
					//}
				}
				[[fallthrough]];
			case S_SpeedTest: // 3
				if (haveBumpedUpMaxStrategyUsed(S_SpeedTest)) {
#ifdef REPORT_RECOVER
					logger() << F("\t\tS_SpeedTest") << L_endl;
#endif
					{ // code block
						strategyStartTime = micros();
						auto speedTest = I2C_SpeedTest(device());
						speedTest.fastest();
						recoveryTime = micros() - strategyStartTime;
						if (speedTest.error() == _OK) {
							strategy().tryAgain(S_TryAgain);
#ifdef REPORT_RECOVER
							logger() << F("\t\tNew Speed ") << device().runSpeed() << L_endl;
#endif
						}
						else {
#ifdef REPORT_RECOVER
							logger() << F("\t\tDevice Failed\n");
#endif
							disable();
						}
						device().i2C().setStopMargin(I2C_Talk::WORKING_STOP_TIMEOUT);
						if (device().testDevice() == _OK) break;
					} // fall-through on error
				} //else logger() << F("\t\tTry again S_SpeedTest 0x");
				[[fallthrough]];
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
				[[fallthrough]];
			case S_Disable: // 5
				if (haveBumpedUpMaxStrategyUsed(S_Disable)) {
#ifdef REPORT_RECOVER
					logger() << L_time << F("S_Disable device 0x") << L_hex << device().getAddress() << L_endl;
#endif
				}			
				if (device().getAddress() == _deviceWaitingOnFailureFor10Mins) { // waiting 10 mins didn't fix it, so do full machine reset
#ifdef REPORT_RECOVER
					logger() << L_time << F("S_DeviceUnrecoverable device 0x") << L_hex << device().getAddress() << L_endl;
#endif
					haveBumpedUpMaxStrategyUsed(S_Unrecoverable);
					_deviceWaitingOnFailureFor10Mins = -_deviceWaitingOnFailureFor10Mins; // signals failed on second attempt
				} else {
					if (_deviceWaitingOnFailureFor10Mins == 0) {
#ifdef REPORT_RECOVER
						logger() << L_time << F("New Disabled device 0x") << L_hex << device().getAddress() << L_endl;
#endif
						_deviceWaitingOnFailureFor10Mins = device().getAddress();
					}
				} // fall-through
				[[fallthrough]];
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
#ifdef REPORT_RECOVER
				logger() << F("\t\tslowdown device 0x") << L_hex << device().getAddress() << F(" speed was ") << L_dec << thisFreq << L_endl;
#endif
				device().set_runSpeed(max(thisFreq - max(thisFreq / 10, 1000), I2C_Talk::MIN_I2C_FREQ));
#ifdef REPORT_RECOVER
				logger() << F("\t\treduced speed to ") << device().runSpeed() << L_endl;
#endif
			}
		//}
		return canReduce;
	}

	void I2C_Recover_Retest::call_timeOutFn(int addr) {
		static bool doingTimeOut;
		if (_timeoutFunctor) {
			if (doingTimeOut) {
#ifdef DEBUG_RECOVER
				logger() << F("\n\t**** call_timeOutFn called recursively ***") << L_endl;
#endif
				return;
			}
			doingTimeOut = true;
			//logger() << F("\ncall_timeOutFn") << L_endl;
			(*_timeoutFunctor)(i2C(), device().getAddress());
		}
		else {
#ifdef DEBUG_RECOVER
			logger() << F("\n\t***  no Time_OutFn set ***") << L_endl;
#endif
			i2C().begin(); // restart i2c in case this is called after an i2c failure
		}
		doingTimeOut = false;
	}
}