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

	Error_codes I2C_Recover_Retest::newReadWrite(I_I2Cdevice_Recovery & i2Cdevice) {
		if (_isRecovering) return i2Cdevice.isEnabled() ? _OK : _disabledDevice;
		//logger() << F("newReadWrite Register device 0x") << L_hex << device().getAddress() << L_endl;
		if (i2Cdevice.reEnable() == _disabledDevice) {
			return _disabledDevice;
		}
		registerDevice(i2Cdevice);
		if (!I2C_SpeedTest::doingSpeedTest()) {
			//logger() << F("newReadWrite setspeed to: ") << device.runSpeed() << L_endl;
			i2C().setI2CFrequency(i2Cdevice.runSpeed());
		}
		return _OK;
	}

	void I2C_Recover_Retest::endReadWrite() {
		if (_isRecovering) return;

		if (_strategy.strategy() == S_NoProblem) {
			if (!I2C_SpeedTest::doingSpeedTest() && _lastGoodDevice == 0) {
				_lastGoodDevice = &device();
				logger() << L_time << F("** new _lastGoodDevice: device 0x") << L_hex << _lastGoodDevice->getAddress() << L_endl << L_endl;
			}
		} else {
			_strategy.succeeded();
		}
	}

	void I2C_Recover_Retest::basicTestsBeforeScan(I_I2Cdevice_Recovery & dev) {
		registerDevice(dev);
	}

	bool I2C_Recover_Retest::tryReadWriteAgain(Error_codes status) {
		static uint32_t recoveryTime;
		static Strategy maxStrategyUsed = S_NoProblem;
				
		// Lambdas
		auto recoveryWasAttempted = []() {return recoveryTime > 0; };
		auto getFinalStrategyRecorded = [this]() {strategy().tryAgain(maxStrategyUsed);};
		auto resetRecoveryStrategy = []() {maxStrategyUsed = S_NoProblem; recoveryTime = 0;};
		auto isRecursiveCall = [this]() {return _isRecovering; };
		auto demoteThisAsLastGoodDevice = [this]() {if (_lastGoodDevice == &device()) _lastGoodDevice = 0; };
		auto haveBumpedUpMaxStrategyUsed = [](Strategy thisStrategy) {if (maxStrategyUsed < thisStrategy) { maxStrategyUsed = thisStrategy; return true; } else return false; };

		uint32_t strategyStartTime;
		bool shouldTryReadingAgain = false;
		//logger() << F("\ntryReadWriteAgain device 0x") << L_hex << device().getAddress() << I2C_Talk::getStatusMsg(status) << L_endl;
		if (status == _OK) {
			if (recoveryWasAttempted()) {
				logger() << L_time << F("Recovery Time uS: ") << recoveryTime << F(" Max Strategy ") << maxStrategyUsed << L_endl;
				auto thisMaxStrategy = maxStrategyUsed;
				(*_timeoutFunctor).postResetInitialisation();
				maxStrategyUsed = thisMaxStrategy;
				getFinalStrategyRecorded();
				resetRecoveryStrategy();
			}
			endReadWrite();
			if (device().getAddress() == abs(_deviceWaitingOnFailureFor10Mins)) _deviceWaitingOnFailureFor10Mins = 0;
		} 
		//else if (status == _BusReleaseTimeout) {
		//	call_timeOutFn(device().getAddress());
		//}
		else if (status == _disabledDevice || isRecursiveCall()) {
			;
		}
		else if (I2C_SpeedTest::doingSpeedTest()) {
			logger() << F("tryReadWriteAgain doingSpeedTest shouldn't come here!") << L_endl;
		} else {
			shouldTryReadingAgain = true;
			demoteThisAsLastGoodDevice();
			logger() << L_time << F("tryReadWriteAgain: device 0x") << L_hex << device().getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << device().runSpeed() << L_endl;
			//if (device().getAddress() == 0x25) return false;
			_strategy.next();
			_isRecovering = true;
			//if (status == _StopMarginTimeout && haveBumpedUpMaxStrategyUsed(S_ExtendStopMargin)) strategy().tryAgain(S_ExtendStopMargin);

			switch (_strategy.strategy()) {
			case S_TryAgain: // 1
			case S_Restart: // 2
				haveBumpedUpMaxStrategyUsed(S_Restart);
				logger() << F("\t\tS_Restart") << L_endl;
				strategyStartTime = micros();
				restart("");
				recoveryTime = micros() - strategyStartTime;
				if (device().testDevice() == _OK) break;
			case S_WaitForDataLine: // 3
				haveBumpedUpMaxStrategyUsed(S_WaitForDataLine);
				logger() << F("\t\tS_WaitForDataLine") << L_endl;
				strategyStartTime = micros();
				//device().i2C().wait_For_I2C_Lines_OK();
				recoveryTime = micros() - strategyStartTime;
				if (device().testDevice() == _OK) break;
				//case S_WaitAgain10:
				//	logger() << F("    S_WaitForDataLine") << L_endl;
				//	strategyStartTime = micros();
				//	Wait_For_I2C_Data_Line_OK();
				//	recoveryTime = micros() - strategyStartTime;
				//	++tryAgain;
				//	if (tryAgain < 20) strategy().tryAgain(S_WaitForDataLine); else tryAgain = 0;
				//	break;
			case S_NotFrozen:
				haveBumpedUpMaxStrategyUsed(S_NotFrozen);
				logger() << F("\t\tS_NotFrozen") << L_endl;
				strategyStartTime = micros();
				if (i2C_is_frozen()) call_timeOutFn(lastGoodDevice()->getAddress());
				recoveryTime = micros() - strategyStartTime;
				//++tryAgain;
				//if (tryAgain < 5) strategy().tryAgain(S_WaitAgain10);
				if (device().testDevice() == _OK) break;
			//case S_ExtendStopMargin:
			//	haveBumpedUpMaxStrategyUsed(S_ExtendStopMargin);
			//	strategyStartTime = micros();
			//	extendStopMargin();
			//	recoveryTime = micros() - strategyStartTime;
			//	logger() << F("\t\tS_ExtendStopMargin to ") << device().i2C().stopMargin() << L_endl;
			//	//++tryAgain;
			//	//if (tryAgain < 5) strategy().tryAgain(S_WaitAgain10);
			//	if (device().testDevice() == _OK) break;
			case S_SlowDown: // 5
				haveBumpedUpMaxStrategyUsed(S_SlowDown);
				logger() << F("\t\tS_Slow-down") << L_endl;
				strategyStartTime = micros();
				{
					auto slowedDown = slowdown();
					recoveryTime = micros() - strategyStartTime;
					if (slowedDown) {
						strategy().tryAgain(S_TryAgain); // keep slowing down until can't.
						break;
					} else {
						logger() << F("\n    Slow-down did nothing...\n");
						if (device().testDevice() == _OK) break;
					}
				}
			case S_SpeedTest: // 6
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
			case S_PowerDown: // 7
				if (haveBumpedUpMaxStrategyUsed(S_PowerDown)) {
					//logger() << F("\t\tS_Power-Down") << L_endl;
					if (status == _BusReleaseTimeout) {
						strategyStartTime = micros();
						call_timeOutFn(device().getAddress());
					}
					//strategy().tryAgain(S_SlowDown); // when incremented will do a speed-test
					if (device().testDevice() == _OK) break;
				} // fall-through
			case S_Disable: // 8
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
			case S_Unrecoverable:
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

	bool I2C_Recover_Retest::i2C_is_frozen() { // try getting status of last know good device
		// preventing recusive calls causes multiple hard resets.
		logger() << F("\n try i2C_is_frozen device 0x") << L_hex << device().getAddress() << L_endl;
		auto i2C_speed = i2C().getI2CFrequency();
		auto status = _OK;
		if (lastGoodDevice() == 0) { // check this device
			status = device().I_I2Cdevice::getStatus();
			logger() << F(" lastGoodDevice() == 0. Check this device") << I2C_Talk::getStatusMsg(status) << L_endl;
		}
		else {
			logger() << F(" Check lastGood device 0x") << L_hex << lastGoodDevice()->getAddress() << I2C_Talk::getStatusMsg(status) << L_endl;
			status = lastGoodDevice()->I_I2Cdevice::getStatus();
		}
		if (status == _BusReleaseTimeout) {
			logger() << F("\n****  i2C_is_frozen **** ")  << L_endl;
		}
		i2C().setI2CFrequency(i2C_speed);
		return status == _BusReleaseTimeout;
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