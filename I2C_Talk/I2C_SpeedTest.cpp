#include "I2C_SpeedTest.h"
#include <Logging.h>

void I2C_SpeedTest::reset() {
	maxSafeSpeed = 0;
	minSafeSpeed = 0;
	error = I2C_Talk::_OK;
}

void I2C_SpeedTest::prepareNextTest() {


	error = I2C_Talk::_OK;
	if (maxSafeSpeed == 0) {
		maxSafeSpeed = I2C_Talk::MAX_I2C_FREQ;
		thisHighestFreq = I2C_Talk::MIN_I2C_FREQ;

	}
	else thisHighestFreq = maxSafeSpeed;
	if (minSafeSpeed == 0) {
		minSafeSpeed = I2C_Talk::MIN_I2C_FREQ;
		thisLowestFreq = I2C_Talk::MAX_I2C_FREQ;
	}
	else thisLowestFreq = minSafeSpeed;
}

template<> // implementation of fully specialized template to test one device
uint32_t I2C_SpeedTest::speedTest_T<false,false>(I_I2Cdevice * deviceFailTest) {
	if (result.foundDeviceAddr > 127) return _I2C_AddressOutOfRange;
	useAutoSpeed(false);
	_inScanOrSpeedTest = true;
	result.prepareNextTest();
	uint8_t restoreNoOfRetries = getNoOfRetries();
	setNoOfRetries(0);
	uint8_t tryAgain = 0;
	//setI2Cfreq_retainAutoSpeed(MAX_I2C_FREQ);
	auto startFreq = getThisI2CFrequency(result.foundDeviceAddr);
	if (startFreq == 0) startFreq = 400000;
	setI2Cfreq_retainAutoSpeed(startFreq);
	result.thisHighestFreq = getI2CFrequency();
	signed char hasFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 1);
	if (hasFailed) hasFailed = findAworkingSpeed(deviceFailTest);
	if (!hasFailed) {
		//Serial.println(" ** findMaxSpeed **");
		hasFailed = findOptimumSpeed(deviceFailTest, result.thisHighestFreq, MAX_I2C_FREQ /*result.maxSafeSpeed*/);
		//Serial.println(" ** findMinSpeed **");
		//hasFailed = hasFailed | findOptimumSpeed(deviceFailTest, result.thisLowestFreq, MIN_I2C_FREQ /*result.minSafeSpeed*/);
	}

	if (hasFailed) {
		result.error = hasFailed; // speed fail
		setThisI2CFrequency(result.foundDeviceAddr, 0); // inhibit future access
		//setThisI2CFrequency(result.foundDeviceAddr, 400000);
	} else {
		setThisI2CFrequency(result.foundDeviceAddr, result.thisHighestFreq);
		//setThisI2CFrequency(result.foundDeviceAddr, (result.thisHighestFreq + result.thisLowestFreq) / 2);
		if (result.thisHighestFreq < result.maxSafeSpeed) result.maxSafeSpeed = result.thisHighestFreq;
		//if (result.thisLowestFreq > result.minSafeSpeed) result.minSafeSpeed = result.thisLowestFreq; 
	}
	setNoOfRetries(restoreNoOfRetries);
	useAutoSpeed(true);
	_inScanOrSpeedTest = false;
	return result.thisHighestFreq;
}

signed char I2C_SpeedTest::testDevice(I_I2Cdevice * deviceFailTest, uint8_t addr, int noOfTestsMustPass) {
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
	if (noOfFailedTests < 2) testFailed = _OK;
	return testFailed;
}

signed char I2C_SpeedTest::findAworkingSpeed(I_I2Cdevice * deviceFailTest) {
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
			if (testFailed == _OK) {
				testFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 1);
				if (testFailed == _OK) {
					result.thisHighestFreq = getI2CFrequency();
					break;
				}
			}
		}
	}
	_lastGoodi2cFreq = getI2CFrequency();
	if (highFailed) result.thisHighestFreq = _lastGoodi2cFreq;
	//if (lowFailed) result.thisLowestFreq = _lastGoodi2cFreq;
	return testFailed ? _I2C_Device_Not_Found : _OK;
}

signed char I2C_SpeedTest::findOptimumSpeed(I_I2Cdevice * deviceFailTest, int32_t & bestSpeed, int32_t limitSpeed ) {
	// enters function at a working frequency
	int32_t adjustBy = (limitSpeed - bestSpeed) /3;
	//Serial.print("\n ** findOptimumSpeed start:"); Serial.print(bestSpeed, DEC); Serial.print(" limit: "); Serial.print(limitSpeed, DEC); Serial.print(" adjustBy: "); Serial.println(adjustBy, DEC);
	//setI2Cfreq_retainAutoSpeed(limitSpeed);
	signed char hasFailed = bestSpeed < limitSpeed; // testDevice(deviceFailTest, result.foundDeviceAddr, 5);
	if (hasFailed) {
		bestSpeed = setI2Cfreq_retainAutoSpeed(limitSpeed - adjustBy);
		hasFailed = 0;
		do {
			//Serial.print("\n Try best speed: "); Serial.print(bestSpeed, DEC); Serial.print(" adjustBy : "); Serial.println(adjustBy, DEC);
			while (!hasFailed && (adjustBy > 0 ? bestSpeed < limitSpeed : bestSpeed > limitSpeed)) { // adjust speed 'till it breaks	
				//Serial.print(" Try at: "); Serial.println(getI2CFrequency(), DEC);
				hasFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 2);
				if (hasFailed) {
					limitSpeed = bestSpeed - adjustBy / 100;
					//Serial.print("\n Failed. NewLimit:"); Serial.println(limitSpeed, DEC);
					callTime_OutFn(result.foundDeviceAddr);
					if (hasFailed == _Timeout) callTime_OutFn(result.foundDeviceAddr);
				}
				else {
					//Serial.print(" OK. BestSpeed was:"); Serial.println(bestSpeed, DEC);
					bestSpeed = setI2Cfreq_retainAutoSpeed(bestSpeed + adjustBy);
				}
			}
			bestSpeed = setI2Cfreq_retainAutoSpeed(bestSpeed - adjustBy);
			hasFailed = 0;
			adjustBy /= 2;
		} while (abs(adjustBy) > bestSpeed/5);
		if (hasFailed == _Timeout) callTime_OutFn(result.foundDeviceAddr);
	}
	hasFailed = adjustSpeedTillItWorksAgain(deviceFailTest, (adjustBy > 0 ? -10 : 10));
	bestSpeed = getI2CFrequency();
	return hasFailed ? _I2C_Device_Not_Found : _OK;
}

signed char I2C_SpeedTest::adjustSpeedTillItWorksAgain(I_I2Cdevice * deviceFailTest, int32_t incrementRatio) {
	constexpr int noOfTestsMustPass = 5;
	signed char hasFailed; // success == 0
	//Serial.print("\n ** Adjust I2C_Speed: "); Serial.println(getI2CFrequency(),DEC);
	_lastGoodi2cFreq = result.thisLowestFreq;
	do { // Adjust speed 'till it works reliably
		hasFailed = testDevice(deviceFailTest,result.foundDeviceAddr, noOfTestsMustPass);
		if (hasFailed) {
			auto increment = getI2CFrequency() / incrementRatio;
			setI2Cfreq_retainAutoSpeed(getI2CFrequency() + increment > 2000 ? increment : 2000);
			//Serial.print(" Adjust I2C_Speed: "); Serial.print(getI2CFrequency(),DEC); Serial.print(" increment :"); Serial.println(getI2CFrequency() / incrementRatio,DEC);
			callTime_OutFn(result.foundDeviceAddr);
			if (hasFailed == _Timeout) callTime_OutFn(result.foundDeviceAddr);
		}
	} while (hasFailed && getI2CFrequency() > MIN_I2C_FREQ && getI2CFrequency() < MAX_I2C_FREQ);
	//Serial.print(" Adjust I2C_Speed "); Serial.print(hasFailed ? "failed at : " : "finished at : "); Serial.println(getI2CFrequency(),DEC);
	return hasFailed ? _I2C_Device_Not_Found : _OK;
}

