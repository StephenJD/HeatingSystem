#include "I2C_Recover.h"
#include <Logging.h>

// Private free function prototypes
bool g_delayTrue(uint8_t millisecs);

void I2C_Recover_Retest::setTimeoutFn (I_I2CresetFunctor * timeOutFn) {timeoutFunctor = timeOutFn;} // Timeout function pointer

void I2C_Recover_Retest::setTimeoutFn (TestFnPtr timeOutFn) { // convert function into functor
	_i2CresetFunctor.set(timeOutFn);
	timeoutFunctor = &_i2CresetFunctor;
} // Timeout function pointer

bool I2C_Recover_Retest::tryReadWriteAgain(uint8_t status, uint16_t deviceAddr) {
	return status && ++noOfFailures < noOfRetries && restart("read 0x", deviceAddr) && g_delayTrue(noOfFailures); // create an increasing delay if we are going to loop
}

void I2C_Recover_Retest::endReadWrite() {
	if (noOfFailures < noOfRetries && noOfFailures > successAfterRetries) successAfterRetries = noOfFailures;
}

bool I2C_Recover_Retest::checkForTimeout(uint8_t status, uint16_t deviceAddr) {
	if (status == I2C_Talk::_Timeout) { 
		callTime_OutFn(deviceAddr);
		//Serial.print("checkForTimeout() failure: "); Serial.println(getError(error));
		return true;
	}
	return false;
}

bool I2C_Recover_Retest::restart(const char * name, int addr) const {
	//Serial.print("I2C::restart() "); Serial.print(name);Serial.println(addr, HEX);
	i2C().restart();
}

uint8_t I2C_Recover_Retest::speedOK(uint16_t deviceAddr) {
	if (i2C().getThisI2CFrequency(deviceAddr) == 0) {
		if (millis() - getFailedTime(deviceAddr) > 600000) {
			i2C().setThisI2CFrequency(deviceAddr, 400000);
		}
		else return I2C_Talk::_speedError;
	}
	else return I2C_Talk::_OK;
}

bool I2C_Recover_Retest::endTransmissionError(uint8_t error, uint16_t deviceAddr) {
	if (error != I2C_Talk::_OK && i2C().usingAutoSpeed()) {
		slowdown_and_reset(deviceAddr);
	}
	//if (status) {
	//	logger().log("check_endTransmissionOK() error:", error, "Auto:",  i2C().usingAutoSpeed());
	//	logger().log("Time since error:", millis() - getFailedTime(addr), "Addr:", addr);
	//}
}





bool I2C_Recover_Retest::slowdown_and_reset(int addr) { // called by check_endTransmissionOK only with auto-speed
	bool canReduce = false;
	if (millis() - getFailedTime(addr) < 10000) { // within 10secs try reducing speed.
		auto thisFreq = i2C().getI2CFrequency();
		canReduce = i2C().getThisI2CFrequency(addr) > I2C_Talk::MIN_I2C_FREQ;
		if (canReduce) {
			logger().log("slowdown_and_reset for", addr, "speed was", thisFreq);
			i2C().setThisI2CFrequency(addr, thisFreq - thisFreq / 10);
			logger().log("slowdown_and_reset for", addr, "reduced speed to", i2C().getI2CFrequency());
		}
		else {
			_speedTest.setI2C_Address(addr);
			_speedTest.speedTest();
			logger().log("slowdown_and_reset called speedTest. Now:", thisFreq);
		}
		//Serial.print("slowdown_and_reset() New speed: "); Serial.println(_i2cFreq,DEC);
	}
	setFailedTime(addr);
	return canReduce;
}

bool I2C_Recover_Retest::i2C_is_frozen(int addr) {
	// preventing recusive calls causes multiple hard resets.
	wireBegin();
	if (addr) {
		wirePort().beginTransmission((uint8_t)addr);
		auto timeOut = micros();
		uint8_t err = wirePort().endTransmission();
		if (micros() - timeOut > 20000) {
			logger().log("****  i2C_is_frozen timed-out  ****");
			if (timeoutFunctor != 0) {
				(*timeoutFunctor)(*this, addr);
			}
		} else if (err != _Timeout){
			//logger().log("****  i2C_is_frozen recovered with endTrans()  ****");
			return false;
		}
	}
	unsigned long downTime = micros() + 20L;
	while (digitalRead(_I2C_DATA_PIN) == LOW && micros() < downTime);
	wireBegin();
	return (digitalRead(_I2C_DATA_PIN) == LOW);
}

void I2C_Recover_Retest::callTime_OutFn(int addr) {
	static bool doingTimeOut;
	if (i2C_is_frozen(addr)) {
		if (timeoutFunctor != 0) {
			if (doingTimeOut) {
				logger().log("callTime_OutFn called recursivly");
				return;
			}
			doingTimeOut = true;
			//Serial.println("call Time_OutFn");
			(*timeoutFunctor)(*this, addr);
		}
		else //Serial.println("... no Time_OutFn set");
			i2C().restart(); // restart i2c in case this is called after an i2c failure
		//delayMicroseconds(5000);
	}
	doingTimeOut = false;
}

signed char I2C_Recover_Retest::testDevice(I_I2Cdevice * deviceFailTest, uint8_t addr, int noOfTestsMustPass) {
	signed char testFailed = 0;
	signed char failed;
	int noOfFailedTests = 0;
	do {
		if (deviceFailTest == 0) {
			//Serial.println("testDevice - none set, using notExists");
			failed = i2C().notExists(addr);
		}
		else {
			failed = deviceFailTest->testDevice();
		}
		if (failed) {
			++noOfFailedTests;
			testFailed |= failed;
		}
		--noOfTestsMustPass;
	} while (noOfFailedTests < 2 && (!testFailed && noOfTestsMustPass > 0) || (testFailed && noOfTestsMustPass >= 0));
	if (noOfFailedTests < 2) testFailed = _OK;
	return testFailed;
}

signed char I2C_Recover_Retest::findAworkingSpeed(I_I2Cdevice * deviceFailTest) {
	// Must test MAX_I2C_FREQ and MIN_I2C_FREQ as these are skipped in later tests
	uint32_t tryFreq[] = {52000,8200,330000,3200,21000,2000,5100,13000,33000,83000,210000};
	//result.thisHighestFreq = setI2Cfreq_retainAutoSpeed(MAX_I2C_FREQ);
	//setI2Cfreq_retainAutoSpeed(100000);
	//setI2Cfreq_retainAutoSpeed(result.maxSafeSpeed);
	int highFailed = testDevice(0, result.foundDeviceAddr, 1);
	if (!highFailed) highFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 1);
	//result.thisLowestFreq = setI2Cfreq_retainAutoSpeed(MIN_I2C_FREQ*2);
	//int lowFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 1);
	int testFailed = highFailed /*|| lowFailed*/;
	if (testFailed) {
		for (auto freq : tryFreq) {
			i2C().setI2Cfreq_retainAutoSpeed(freq);
			//logger().log("Trying addr:", result.foundDeviceAddr,"at freq:",freq);// Serial.print(" Success: "); Serial.println(getError(testFailed));
			auto timeOut = micros();
			testFailed = testDevice(0, result.foundDeviceAddr, 1);
			if (micros() - timeOut > 20000) {
				logger().log("****  findAworkingSpeed timed-out   at freq:", freq );
				if (timeoutFunctor != 0) {
					(*timeoutFunctor)(*this, result.foundDeviceAddr);
				}
			}
			//logger().log(" TestDevice time:", micros()-timeOut);
			if (testFailed == _OK) {
				testFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 1);
				if (testFailed == _OK) {
					result.thisHighestFreq = i2C().getI2CFrequency();
					break;
				}
			}
		}
	}
	_lastGoodi2cFreq = i2C().getI2CFrequency();
	if (highFailed) result.thisHighestFreq = _lastGoodi2cFreq;
	//if (lowFailed) result.thisLowestFreq = _lastGoodi2cFreq;
	return testFailed ? _I2C_Device_Not_Found : _OK;
}

signed char I2C_Recover_Retest::adjustSpeedTillItWorksAgain(I_I2Cdevice * deviceFailTest, int32_t incrementRatio) {
	constexpr int noOfTestsMustPass = 5;
	signed char hasFailed; // success == 0
	//Serial.print("\n ** Adjust I2C_Speed: "); Serial.println(getI2CFrequency(),DEC);
	_lastGoodi2cFreq = result.thisLowestFreq;
	do { // Adjust speed 'till it works reliably
		hasFailed = testDevice(deviceFailTest,result.foundDeviceAddr, noOfTestsMustPass);
		if (hasFailed) {
			auto increment = i2C().getI2CFrequency() / incrementRatio;
			i2C().setI2Cfreq_retainAutoSpeed(i2C().getI2CFrequency() + increment > 2000 ? increment : 2000);
			//Serial.print(" Adjust I2C_Speed: "); Serial.print(getI2CFrequency(),DEC); Serial.print(" increment :"); Serial.println(getI2CFrequency() / incrementRatio,DEC);
			callTime_OutFn(result.foundDeviceAddr);
			if (hasFailed == _Timeout) callTime_OutFn(result.foundDeviceAddr);
		}
	} while (hasFailed && i2C().getI2CFrequency() > I2C_Talk::MIN_I2C_FREQ && i2C().getI2CFrequency() < I2C_Talk::MAX_I2C_FREQ);
	//Serial.print(" Adjust I2C_Speed "); Serial.print(hasFailed ? "failed at : " : "finished at : "); Serial.println(getI2CFrequency(),DEC);
	return hasFailed ? _I2C_Device_Not_Found : _OK;
}

// ************** Private Free Functions ************
bool g_delayTrue(uint8_t millisecs) { // wrapper to get a return true for use in short-circuited test
	delay(millisecs);
	return true;
}


