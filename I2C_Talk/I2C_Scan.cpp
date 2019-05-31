#include "I2C_Scan.h"
#include <I2C_Talk.h>
#include <Logging.h>

void I2C_Scan::reset(){
	foundDeviceAddr = -1;
	totalDevicesFound = 0;
    error = I2C_Talk::_OK;
}

bool I2C_Scan::nextDevice() { // returns false when no more found
	if (!_i2C->_canWrite) return false;
	_i2C->_inScanOrSpeedTest = true;
	_i2C->useAutoSpeed(false);
	if (foundDeviceAddr == -1) {
		totalDevicesFound = 0;
		foundDeviceAddr = 0;
	}
	while(++foundDeviceAddr >= 0) {
		callTime_OutFn(foundDeviceAddr);
		auto startFreq = _i2C->getThisI2CFrequency(foundDeviceAddr);
		if (startFreq == 0) startFreq = 100000;
		startFreq = _i2C->setI2Cfreq_retainAutoSpeed(startFreq);
		thisHighestFreq = startFreq;
		//Serial.print(result.foundDeviceAddr, HEX); Serial.print(" at freq: "); Serial.println(startFreq, DEC);

		error = findAworkingSpeed(0);
		if (error == I2C_Talk::_OK) {
		  ++totalDevicesFound;
		  break;
		} else if (error == I2C_Talk::_NACK_during_complete) break; // unknown bus error
	}
	if (error == I2C_Talk::_NACK_during_address_send) error = I2C_Talk::_OK; // don't report expected errors whilst searching for a device
	_i2C->useAutoSpeed(true);
	_i2C->_inScanOrSpeedTest = false;
	return (foundDeviceAddr >= 0);
}

void I2C_Scan::callTime_OutFn(int addr) {
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
			restart(); // restart i2c in case this is called after an i2c failure
		//delayMicroseconds(5000);
	}
	doingTimeOut = false;
}

signed char I2C_Scan::testDevice(I_I2Cdevice * deviceFailTest, uint8_t addr, int noOfTestsMustPass) {
	signed char testFailed = 0;
	signed char failed;
	int noOfFailedTests = 0;
	do {
		if (deviceFailTest == 0) {
			//Serial.println("testDevice - none set, using notExists");
			failed = notExists(*this, addr);
		}
		else {
			failed = deviceFailTest->testDevice(*this, addr);
		}
		if (failed) {
			++noOfFailedTests;
			testFailed |= failed;
		}
		--noOfTestsMustPass;
	} while (noOfFailedTests < 2 && (!testFailed && noOfTestsMustPass > 0) || (testFailed && noOfTestsMustPass >= 0));
	if (noOfFailedTests < 2) testFailed = I2C_Talk::_OK;
	return testFailed;
}

signed char I2C_Helper::findAworkingSpeed(I_I2Cdevice * deviceFailTest) {
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
			setI2Cfreq_retainAutoSpeed(freq);
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
			if (testFailed == I2C_Talk::_OK) {
				testFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 1);
				if (testFailed == I2C_Talk::_OK) {
					result.thisHighestFreq = getI2CFrequency();
					break;
				}
			}
		}
	}
	_lastGoodi2cFreq = getI2CFrequency();
	if (highFailed) result.thisHighestFreq = _lastGoodi2cFreq;
	//if (lowFailed) result.thisLowestFreq = _lastGoodi2cFreq;
	return testFailed ? _I2C_Device_Not_Found : I2C_Talk::_OK;
}



