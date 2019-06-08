#include "I2C_RecoverRetest.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>
#include <..\Logging\Logging.h>

using namespace I2C_Talk_ErrorCodes;

void I2C_Recover_Retest::setTimeoutFn (I_I2CresetFunctor * timeOutFn) {timeoutFunctor = timeOutFn;} // Timeout function pointer

void I2C_Recover_Retest::setTimeoutFn (TestFnPtr timeOutFn) { // convert function into functor
	_i2CresetFunctor.set(timeOutFn);
	timeoutFunctor = &_i2CresetFunctor;
} // Timeout function pointer

void I2C_Recover_Retest::newReadWrite() { 
	noOfFailures = 0;
	if (device().runSpeed() == 0) {
		if (millis() - device().getFailedTime() > DISABLE_PERIOD_ON_FAILURE) {
			device().set_runSpeed(START_SPEED_AFTER_FAILURE);
		}
		else return;
	}
	i2C().setI2CFrequency(device().runSpeed());
}

bool I2C_Recover_Retest::tryReadWriteAgain(uint8_t status) {
	if (status != _OK) {
		if (status == _Timeout) {
			ensureNotFrozen();
			//Serial.print("checkForTimeout() failure: "); Serial.println(getStatusMsg(error));
			return true;
		}
		switch (_strategy) {
		case S_delay:
			strategey1();
			++noOfFailures;
			if (noOfFailures > maxRetries) ++_strategy;
			return true;
		case S_restart:
			strategey2();
			++_strategy;
			return true;
		case S_SlowDown:
			strategey3();
			++_strategy;
			return true;
		case S_SpeedTest:
			strategey4();
			++_strategy;
			return true;
		case S_PowerDown:
			strategey5();
			++_strategy;
			return true;
		case S_Disable:
			strategey6();
			++_strategy;
			return false;
		}
	}
	return false;
}

void I2C_Recover_Retest::endReadWrite() {
	if (noOfFailures < maxRetries && noOfFailures > successAfterRetries) successAfterRetries = noOfFailures;
}

bool I2C_Recover_Retest::restart(const char * name) const {
	//Serial.print("I2C::restart() "); Serial.print(name);Serial.println(addr(), HEX);
	return i2C().restart();
}

bool I2C_Recover_Retest::slowdown() { // called by failure strategy
	bool canReduce = false;
	if (millis() - device().getFailedTime() < REPEAT_FAILURE_PERIOD) { // within 10secs try reducing speed.
		auto thisFreq = device().runSpeed();
		canReduce = thisFreq > I2C_Talk::MIN_I2C_FREQ;
		if (canReduce) {
			logger().log("slowdown for", device().getAddress(), "speed was", thisFreq);
			device().set_runSpeed(max(thisFreq - max(thisFreq / 10, 1000), I2C_Talk::MIN_I2C_FREQ));
			logger().log("reduced speed to", device().runSpeed());
		}
	}
	device().setFailedTime();
	return canReduce;
}

bool I2C_Recover_Retest::i2C_is_frozen() {
	// preventing recusive calls causes multiple hard resets.
	wireBegin();
	auto addr = device().getAddress();

	wirePort().beginTransmission((uint8_t)addr);
	auto timeOut = micros();
	uint8_t err = wirePort().endTransmission();
	if (micros() - timeOut > TIMEOUT) {
		logger().log("****  i2C_is_frozen timed-out  ****");
		if (timeoutFunctor != 0) {
			(*timeoutFunctor)(i2C(), addr);
		}
	} else if (err != _Timeout){
		//logger().log("****  i2C_is_frozen recovered with endTrans()  ****");
		return false;
	}

	unsigned long downTime = micros() + 20L;
	while (digitalRead(_I2C_DATA_PIN) == LOW && micros() < downTime);
	wireBegin();
	return (digitalRead(_I2C_DATA_PIN) == LOW);
}

void I2C_Recover_Retest::ensureNotFrozen() {
	static bool doingTimeOut;
	if (i2C_is_frozen()) {
		if (timeoutFunctor != 0) {
			if (doingTimeOut) {
				logger().log("ensureNotFrozen called recursivly");
				return;
			}
			doingTimeOut = true;
			//Serial.println("call Time_OutFn");
			(*timeoutFunctor)(i2C(), device().getAddress());
		}
		else //Serial.println("... no Time_OutFn set");
			i2C().restart(); // restart i2c in case this is called after an i2c failure
		//delayMicroseconds(5000);
	}
	doingTimeOut = false;
}

uint8_t I2C_Recover_Retest::notExists() {
	auto status = device().getStatus();
	if (status == _OK) {
		status = i2C().notExists(device().getAddress());
		ensureNotFrozen();
	}
	return status;
}

uint8_t I2C_Recover_Retest::testDevice(int noOfTests, int maxNoOfFailures) {
	uint8_t return_status = _OK;
	uint8_t status;
	do {
		status = device().testDevice();
		if (status != _OK) {
			--maxNoOfFailures;
			return_status = status;
		}
		--noOfTests;
		ensureNotFrozen();
	} while (noOfTests-maxNoOfFailures);
	if (maxNoOfFailures > 0) return _OK; else return return_status;
}

uint8_t I2C_Recover_Retest::findAworkingSpeed(I_I2Cdevice & i2c_device) {
	registerDevice(i2c_device);
	auto addr = i2c_device.getAddress();
	// Must test MAX_I2C_FREQ as this is skipped in later tests
	constexpr uint32_t tryFreq[] = {52000,8200,330000,3200,21000,2000,5100,13000,33000,83000,210000};
	//Serial.println("findAworkingSpeed-Exists"); Serial.flush();
	i2C().setI2CFrequency(I2C_Talk::MAX_I2C_FREQ);
	auto highFailed = notExists();
	if (!highFailed) {
		Serial.print("findAworkingSpeed Test (device) at "); Serial.println(i2C().getI2CFrequency());
		highFailed = testDevice(2, 1);
	}

	int testFailed = highFailed;
	if (testFailed) {
		for (auto freq : tryFreq) {
			i2C().setI2CFrequency(freq);
			//logger().log("Trying addr:", foundDeviceAddr,"at freq:",freq);// Serial.print(" Success: "); Serial.println(getStatusMsg(testFailed));
			testFailed = notExists();

			if (testFailed == _OK) {
				Serial.print("findAworkingSpeed Test (device) at "); Serial.println(freq);
				testFailed = testDevice(2,1);
				if (testFailed == _OK) break;
			}
		}
	}
	_lastGoodi2cFreq = i2C().getI2CFrequency();
	return testFailed ? _I2C_Device_Not_Found : _OK;
}