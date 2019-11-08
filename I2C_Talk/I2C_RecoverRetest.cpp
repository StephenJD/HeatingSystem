#include "I2C_RecoverRetest.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>
//#include <Logging.h>
#include <..\Logging\Logging.h>

void ui_yield();
extern int st_index;

namespace I2C_Recovery {

	using namespace I2C_Talk_ErrorCodes;

	error_codes I2C_Recover_Retest::testDevice(int noOfTests, int allowableFailures) {
		auto return_status = _OK;
		auto status = _OK;
		do {
			status = device().testDevice();
			if (status != _OK) {
				--allowableFailures;
				return_status = status;
			}
			--noOfTests;
			//logger() << "testDevice Tests/Failures ", noOfTests, "/", allowableFailures);
		} while (allowableFailures >= 0 && noOfTests > 0);
		if (allowableFailures >= 0) return _OK; else return return_status;
	}

	error_codes I2C_Recover_Retest::findAworkingSpeed() {
		if (device().getStatus() == _disabledDevice) return _disabledDevice;
		auto addr = device().getAddress();
		// Must test MAX_I2C_FREQ as this is skipped in later tests
		constexpr uint32_t tryFreq[] = { 52000,8200,330000,3200,21000,2000,5100,13000,33000,83000,210000 };
		i2C().setI2CFrequency(I2C_Talk::MAX_I2C_FREQ);
		//logger() << "findAworkingSpeed Try Exists? At:", i2C().getI2CFrequency());
		auto testResult = i2C().status(addr);
		//logger() << "findAworkingSpeed Exists? At:", i2C().getI2CFrequency(), device().getStatusMsg(testResult));
		if (testResult == _OK) {
			testResult = testDevice(2, 1);
			logger() << L_time << "findAworkingSpeed Exists succeeded. Test device at " << i2C().getI2CFrequency() << device().getStatusMsg(testResult);
		}

		if (testResult != _OK) {
			for (auto freq : tryFreq) {
				i2C().setI2CFrequency(freq);
				logger() << "\nTrying addr: " << addr << " at freq: " << freq;// Serial.print(" Success: "); Serial.println(getStatusMsg(testResult);
				testResult = i2C().status(addr);

				if (testResult == _OK) {
					device().set_runSpeed(freq);
					testResult = testDevice(2, 1);
					logger() << "\nfindAworkingSpeed Exists succeeded. Test device at " << i2C().getI2CFrequency() << device().getStatusMsg(testResult);
					if (testResult == _OK) break;
				}
				ui_yield();
			}
		}
		return testResult ? _I2C_Device_Not_Found : _OK;
	}

	void I2C_Recover_Retest::setTimeoutFn(I_I2CresetFunctor * timeOutFn) { _timeoutFunctor = timeOutFn; } // Timeout function pointer

	void I2C_Recover_Retest::setTimeoutFn(TestFnPtr timeOutFn) { // convert function into functor
		_i2CresetFunctor.set(timeOutFn);
		_timeoutFunctor = &_i2CresetFunctor;
	} // Timeout function pointer

	error_codes I2C_Recover_Retest::newReadWrite(I_I2Cdevice_Recovery & device) {
		if (_isRecovering) return device.isEnabled() ? _OK : _disabledDevice;
		registerDevice(device);
		if (!device.isEnabled()) {
			if (millis() - device.getFailedTime() > DISABLE_PERIOD_ON_FAILURE) {
				device.reset();
				logger() << "\n10s rest: Re-enabling disabled device " << device.getAddress();
			}
			else {
				logger() << "\nDisabled device" << device.getAddress();
				return _disabledDevice;
			}
		}
		if (!I2C_SpeedTest::doingSpeedTest()) i2C().setI2CFrequency(device.runSpeed());
		return _OK;
	}

	void I2C_Recover_Retest::endReadWrite() {
		if (_isRecovering) return;

		if (_strategy.strategy() == S_NoProblem) {
			if (!I2C_SpeedTest::doingSpeedTest() && _lastGoodDevice == 0) {
				_lastGoodDevice = &device();
				logger() << "\n_lastGoodDevice reset to " << _lastGoodDevice->getAddress();
			}
		} else {
			_strategy.succeeded();
		}
	}

	void I2C_Recover_Retest::basicTestsBeforeScan() {
		Wait_For_I2C_Data_Line_OK();
		if (i2C_is_frozen()) call_timeOutFn(device().getAddress());
	}

	bool I2C_Recover_Retest::tryReadWriteAgain(error_codes status) {
		static uint32_t recoveryTime;
		uint32_t strategyStartTime;
		static int tryAgain;
		//logger() << "\ntryReadWriteAgain: status" << status;
		if (status == _OK) {
			tryAgain = 0;
			if (recoveryTime > 0) {
				logger() << "\nRecovery Time uS: " << recoveryTime;
				strategy().stackTrace(++st_index, "tryReadWriteAgain - recovered");
				recoveryTime = 0;
			}
			endReadWrite();
			return false;
		}
		else if (status == _disabledDevice || _isRecovering) {
			return false;
		} else if (I2C_SpeedTest::doingSpeedTest()) {
			if (i2C_is_frozen()) call_timeOutFn(lastGoodDevice()->getAddress());
			return false;	
		} else {
			if (_lastGoodDevice == &device()) _lastGoodDevice = 0;
			//logger() << "\ntryReadWriteAgain: " << device().getAddress() << device().getStatusMsg(status);
			_strategy.next();
			strategy().stackTrace(++st_index, "tryReadWriteAgain - next");
			_isRecovering = true;
			switch (_strategy.strategy()) {
			case S_TryAgain:
			case S_Restart:
				logger() << "\n    S_Restart " << device().getAddress() << device().getStatusMsg(status);
				strategyStartTime = micros();
				restart("read 0x");
				recoveryTime = micros() - strategyStartTime;
				strategy().stackTrace(++st_index, "S_Restart");
				break;			
			case S_WaitForDataLine:
				logger() << "\n    S_WaitForDataLine" << device().getAddress() << device().getStatusMsg(status);
				strategyStartTime = micros();
				Wait_For_I2C_Data_Line_OK();
				recoveryTime = micros() - strategyStartTime;
				strategy().stackTrace(++st_index, "S_WaitForDataLine");
				break;
			//case S_WaitAgain10:
			//	logger() << "    S_WaitForDataLine", device().getAddress(), device().getStatusMsg(status));
			//	strategyStartTime = micros();
			//	Wait_For_I2C_Data_Line_OK();
			//	recoveryTime = micros() - strategyStartTime;
			//	strategy().stackTrace(++st_index, "Wait_For_I2C_Data_Line_OK");
			//	++tryAgain;
			//	if (tryAgain < 20) strategy().tryAgain(S_WaitForDataLine); else tryAgain = 0;
			//	break;
			case S_NotFrozen:
				logger() << "\n    S_NotFrozen " << device().getAddress() << device().getStatusMsg(status);
				strategyStartTime = micros();
				if (i2C_is_frozen()) call_timeOutFn(lastGoodDevice()->getAddress());
				recoveryTime = micros() - strategyStartTime;
				strategy().stackTrace(++st_index, "S_NotFrozen");
				//++tryAgain;
				//if (tryAgain < 5) strategy().tryAgain(S_WaitAgain10);
				break;
			case S_SlowDown:
				logger() << "\n    S_Slow-down " << device().getAddress() << device().getStatusMsg(status);
				strategyStartTime = micros();
				{ auto slowedDown = slowdown();
					recoveryTime = micros() - strategyStartTime;
					if (!slowedDown) logger() << "\n    Slow-down did nothing...";
				}
				strategy().stackTrace(++st_index, "S_SlowDown");
				break;
			case S_SpeedTest:
				logger() << "\n    S_SpeedTest " << device().getAddress() << device().getStatusMsg(status);
				strategyStartTime = micros();
				{
					auto speedTest = I2C_SpeedTest(device());
					speedTest.fastest();
					recoveryTime = micros() - strategyStartTime;
					if (speedTest.error())
						logger() << "\n   Device Failed";
					else {
						strategy().tryAgain(S_TryAgain);
						logger() << "\n   New Speed " << device().runSpeed();
					}
				}
				strategy().stackTrace(++st_index, "S_SpeedTest");
				break;
			case S_PowerDown:
				logger() << "\n    S_Power-Down " << device().getAddress() << device().getStatusMsg(status);
				strategyStartTime = micros();
				call_timeOutFn(device().getAddress());
				{
					auto speedTest = I2C_SpeedTest(device());
					speedTest.fastest();
					recoveryTime = micros() - strategyStartTime;
					if (speedTest.error())
						logger() << "\n   Device Failed";
					else {
						strategy().tryAgain(S_TryAgain);
						logger() << "\n   New Speed " << device().runSpeed();
					}
				}
				strategy().stackTrace(++st_index, "S_PowerDown");
				++tryAgain;
				if (tryAgain <= 1) strategy().tryAgain(S_TryAgain);
				break;
			case S_Disable:
				logger() << "\n    S_Disable Device " << device().getAddress() << device().getStatusMsg(status);
				disable();
				strategy().stackTrace(++st_index, "S_Disable");
				recoveryTime = 0;
				_isRecovering = false;
				endReadWrite();
				return false;
			default:
				_isRecovering = false;
				return false;
			}
			_isRecovering = false;
			return true;
		}
	}

	bool I2C_Recover_Retest::restart(const char * name) {
		wireBegin();
		return true;
	}

	bool I2C_Recover_Retest::slowdown() { // called by failure strategy
		bool canReduce = false;
		if (millis() - device().getFailedTime() < REPEAT_FAILURE_PERIOD) { // within 10secs try reducing speed.
			auto thisFreq = device().runSpeed();
			canReduce = thisFreq > I2C_Talk::MIN_I2C_FREQ;
			if (canReduce) {
				logger() << "\n\tslowdown for " << device().getAddress() << " speed was " << thisFreq;
				device().set_runSpeed(max(thisFreq - max(thisFreq / 10, 1000), I2C_Talk::MIN_I2C_FREQ));
				logger() << "\n\treduced speed to " << device().runSpeed();
			}
		}
		return canReduce;
	}

	bool I2C_Recover_Retest::Wait_For_I2C_Data_Line_OK() {
		unsigned long downTime = micros() + 20L;
		while (digitalRead(_I2C_DATA_PIN) == LOW && micros() < downTime);
		wireBegin();
		return (digitalRead(_I2C_DATA_PIN) == LOW);
	}

	bool I2C_Recover_Retest::i2C_is_frozen() {
		// preventing recusive calls causes multiple hard resets.
		if (lastGoodDevice() == 0) {
			if (device().getStatus()) call_timeOutFn(device().getAddress());  
			return false; 
		}

		auto runTime = micros();
		auto i2C_speed = i2C().getI2CFrequency();
		auto status = lastGoodDevice()->getStatus();
		i2C().setI2CFrequency(i2C_speed);
		runTime = micros() - runTime;
		if (status == _Timeout || runTime > TIMEOUT) { // 20mS timeout
			logger() << "\n****  i2C_is_frozen timed-out  **** " << lastGoodDevice()->getAddress();
			return true;
		}
		return false;
	}

	void I2C_Recover_Retest::call_timeOutFn(int addr) {
		static bool doingTimeOut;
		if (_timeoutFunctor != 0) {
			if (doingTimeOut) {
				logger() << "\n\tcall_timeOutFn called recursively";
				return;
			}
			doingTimeOut = true;
			(*_timeoutFunctor)(i2C(), device().getAddress());
		}
		else {
			logger() << "\n\t***  no Time_OutFn set ***\n";
			i2C().restart(); // restart i2c in case this is called after an i2c failure
		}
		doingTimeOut = false;
	}
}