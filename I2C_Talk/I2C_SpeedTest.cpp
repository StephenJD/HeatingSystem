#include "I2C_SpeedTest.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <Logging.h>



void I2C_SpeedTest::reset() {
	maxSafeSpeed = 0;
	//minSafeSpeed = 0;
	error = I2C_Talk::_OK;
}

void I2C_SpeedTest::prepareNextTest() {
	error = I2C_Talk::_OK;
	if (maxSafeSpeed == 0) {
		maxSafeSpeed = I2C_Talk::MAX_I2C_FREQ;
		thisHighestFreq = I2C_Talk::MIN_I2C_FREQ;
	}
	else thisHighestFreq = maxSafeSpeed;
	//if (minSafeSpeed == 0) {
	//	minSafeSpeed = I2C_Talk::MIN_I2C_FREQ;
	//	thisLowestFreq = I2C_Talk::MAX_I2C_FREQ;
	//}
	//else thisLowestFreq = minSafeSpeed;
}

template<> // implementation of fully specialized template to test one device
uint32_t I2C_SpeedTest::speedTest_T<false,false>(I_I2Cdevice * deviceFailTest) {
	if (_devAddr > 127) return I2C_Talk::_I2C_AddressOutOfRange;
	_i2C->useAutoSpeed(false);
	_inScanOrSpeedTest = true;
	prepareNextTest();
	//uint8_t restoreNoOfRetries = getNoOfRetries();
	//setNoOfRetries(0);
	uint8_t tryAgain = 0;

	auto startFreq = _i2C->getThisI2CFrequency(_devAddr);
	if (startFreq == 0) startFreq = 400000;
	_i2C->setI2Cfreq_retainAutoSpeed(startFreq);
	thisHighestFreq = _i2C->getI2CFrequency();
	signed char hasFailed = testDevice(deviceFailTest, _devAddr, 1);
	if (hasFailed) hasFailed = _i2C->recovery().findAworkingSpeed(deviceFailTest);
	if (!hasFailed) {
		//Serial.println(" ** findMaxSpeed **");
		hasFailed = findOptimumSpeed(deviceFailTest, thisHighestFreq, I2C_Talk::MAX_I2C_FREQ /*result.maxSafeSpeed*/);
		//Serial.println(" ** findMinSpeed **");
		//hasFailed = hasFailed | findOptimumSpeed(deviceFailTest, result.thisLowestFreq, MIN_I2C_FREQ /*result.minSafeSpeed*/);
	}

	if (hasFailed) {
		error = hasFailed; // speed fail
		_i2C->setThisI2CFrequency(_devAddr, 0); // inhibit future access
		//setThisI2CFrequency(_devAddr, 400000);
	} else {
		_i2C->setThisI2CFrequency(_devAddr, thisHighestFreq);
		//setThisI2CFrequency(_devAddr, (result.thisHighestFreq + result.thisLowestFreq) / 2);
		if (thisHighestFreq < maxSafeSpeed) maxSafeSpeed = thisHighestFreq;
		//if (result.thisLowestFreq > result.minSafeSpeed) result.minSafeSpeed = result.thisLowestFreq; 
	}
	//setNoOfRetries(restoreNoOfRetries);
	_i2C->useAutoSpeed(true);
	_inScanOrSpeedTest = false;
	return thisHighestFreq;
}

signed char I2C_SpeedTest::testDevice(I_I2Cdevice * deviceFailTest, int16_t addr, int noOfTestsMustPass) {
	signed char testFailed = 0;
	signed char failed;
	int noOfFailedTests = 0;
	do {
		if (deviceFailTest == 0) {
			//Serial.println("testDevice - none set, using notExists");
			failed = _i2C->notExists(addr);
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
	if (noOfFailedTests < 2) testFailed = I2C_Talk::_OK;
	return testFailed;
}

signed char I2C_SpeedTest::findOptimumSpeed(I_I2Cdevice * deviceFailTest, int32_t & bestSpeed, int32_t limitSpeed ) {
	// enters function at a working frequency
	int32_t adjustBy = (limitSpeed - bestSpeed) /3;
	//Serial.print("\n ** findOptimumSpeed start:"); Serial.print(bestSpeed, DEC); Serial.print(" limit: "); Serial.print(limitSpeed, DEC); Serial.print(" adjustBy: "); Serial.println(adjustBy, DEC);
	//setI2Cfreq_retainAutoSpeed(limitSpeed);
	signed char hasFailed = bestSpeed < limitSpeed; // testDevice(deviceFailTest, _devAddr, 5);
	if (hasFailed) {
		bestSpeed = _i2C->setI2Cfreq_retainAutoSpeed(limitSpeed - adjustBy);
		hasFailed = 0;
		do {
			//Serial.print("\n Try best speed: "); Serial.print(bestSpeed, DEC); Serial.print(" adjustBy : "); Serial.println(adjustBy, DEC);
			while (!hasFailed && (adjustBy > 0 ? bestSpeed < limitSpeed : bestSpeed > limitSpeed)) { // adjust speed 'till it breaks	
				//Serial.print(" Try at: "); Serial.println(getI2CFrequency(), DEC);
				hasFailed = testDevice(deviceFailTest, _devAddr, 2);
				if (hasFailed) {
					limitSpeed = bestSpeed - adjustBy / 100;
					//Serial.print("\n Failed. NewLimit:"); Serial.println(limitSpeed, DEC);
					_i2C->recovery().checkForTimeout(hasFailed);
				}
				else {
					//Serial.print(" OK. BestSpeed was:"); Serial.println(bestSpeed, DEC);
					bestSpeed = _i2C->setI2Cfreq_retainAutoSpeed(bestSpeed + adjustBy);
				}
			}
			bestSpeed = _i2C->setI2Cfreq_retainAutoSpeed(bestSpeed - adjustBy);
			hasFailed = 0;
			adjustBy /= 2;
		} while (abs(adjustBy) > bestSpeed/5);
		_i2C->recovery().checkForTimeout(hasFailed);
	}
	hasFailed = adjustSpeedTillItWorksAgain(deviceFailTest, (adjustBy > 0 ? -10 : 10));
	bestSpeed = _i2C->getI2CFrequency();
	return hasFailed ? I2C_Talk::_I2C_Device_Not_Found : I2C_Talk::_OK;
}

signed char I2C_SpeedTest::adjustSpeedTillItWorksAgain(I_I2Cdevice * deviceFailTest, int32_t incrementRatio) {
	constexpr int noOfTestsMustPass = 5;
	signed char hasFailed; // success == 0
	//Serial.print("\n ** Adjust I2C_Speed: "); Serial.println(getI2CFrequency(),DEC);
	//_lastGoodi2cFreq = thisLowestFreq;
	do { // Adjust speed 'till it works reliably
		hasFailed = testDevice(deviceFailTest,_devAddr, noOfTestsMustPass);
		if (hasFailed) {
			auto increment = _i2C->getI2CFrequency() / incrementRatio;
			_i2C->setI2Cfreq_retainAutoSpeed(_i2C->getI2CFrequency() + increment > 2000 ? increment : 2000);
			//Serial.print(" Adjust I2C_Speed: "); Serial.print(getI2CFrequency(),DEC); Serial.print(" increment :"); Serial.println(getI2CFrequency() / incrementRatio,DEC);
			_i2C->recovery().checkForTimeout(hasFailed);
		}
	} while (hasFailed && _i2C->getI2CFrequency() > I2C_Talk::MIN_I2C_FREQ && _i2C->getI2CFrequency() < I2C_Talk::MAX_I2C_FREQ);
	//Serial.print(" Adjust I2C_Speed "); Serial.print(hasFailed ? "failed at : " : "finished at : "); Serial.println(getI2CFrequency(),DEC);
	return hasFailed ? I2C_Talk::_I2C_Device_Not_Found : I2C_Talk::_OK;
}

