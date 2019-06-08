#include "I2C_SpeedTest.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
//#include <Logging.h>

using namespace I2C_Talk_ErrorCodes;

int8_t I2C_SpeedTest::prepareTestAll() {
	maxSafeSpeed = 0;
	prepareNextTest();
	return 0;
}

void I2C_SpeedTest::prepareNextTest() {
	error = _OK;
	if (maxSafeSpeed == 0) {
		maxSafeSpeed = I2C_Talk::MAX_I2C_FREQ;
	}
	thisHighestFreq = I2C_Talk::MAX_I2C_FREQ;
}

template<> // implementation of fully specialized template to test one device
uint32_t I2C_SpeedTest::fastest_T<false,false>(I_I2Cdevice & i2c_device) {
	//if (_devAddr < 1 || _devAddr > 127) {
	//	error = _I2C_AddressOutOfRange;
	//	return 0;
	//}

	inScan(true);
	prepareNextTest();
	recovery().registerDevice(i2c_device);
	auto startFreq = i2c_device.runSpeed();
	if (startFreq == 0) startFreq = 400000;
	//Serial.println(startFreq);

	thisHighestFreq = i2C_Talk().setI2CFrequency(startFreq);
	int8_t hasFailed = recovery().testDevice(2,1);
	//Serial.print("fastest_T<false,false> Device tested. Failed?"); Serial.print(hasFailed); Serial.print(" for 0x"); Serial.println(i2C_Talk().recovery().addr(), HEX); Serial.flush();
	if (hasFailed) hasFailed = recovery().findAworkingSpeed(i2c_device);
	if (!hasFailed) {
		//Serial.println(" ** findMaxSpeed **");
		hasFailed = findOptimumSpeed(i2c_device, thisHighestFreq, I2C_Talk::MAX_I2C_FREQ /*result.maxSafeSpeed*/);
	}

	if (hasFailed) {
		error = hasFailed; // speed fail
		//Serial.print("fastest_T<false,false> setSpeed 0");
		i2c_device.set_runSpeed(0); // inhibit future access
		//set_runSpeed(_devAddr, 400000);
	} else {
		i2c_device.set_runSpeed(thisHighestFreq);
		if (thisHighestFreq < maxSafeSpeed) maxSafeSpeed = thisHighestFreq;
	}
	inScan(false);
	//Serial.print("fastest_T<false,false> Finished");Serial.flush();
	return thisHighestFreq;
}

signed char I2C_SpeedTest::findOptimumSpeed(I_I2Cdevice & i2c_device, int32_t & bestSpeed, int32_t limitSpeed ) {
	// enters function at a working frequency
	int32_t adjustBy = (limitSpeed - bestSpeed) /3;
	//Serial.print("\n ** findOptimumSpeed start:"); Serial.print(i2C_Talk().getI2CFrequency(), DEC); Serial.print(" limit: "); Serial.print(limitSpeed, DEC); Serial.print(" adjustBy: "); Serial.println(adjustBy, DEC); Serial.flush();
	i2C_Talk().setI2CFrequency(limitSpeed);
	auto hasFailed = recovery().testDevice(2,1);
	//Serial.print("\n ** findOptimumSpeed Check?:"); Serial.println(hasFailed);
	if (hasFailed) {
		bestSpeed = i2C_Talk().setI2CFrequency(limitSpeed - adjustBy);
		hasFailed = 0;
		do {
			//Serial.print("\n Try best speed: "); Serial.print(bestSpeed, DEC); Serial.print(" adjustBy : "); Serial.println(adjustBy, DEC); Serial.flush();
			while (!hasFailed && (adjustBy > 0 ? bestSpeed < limitSpeed : bestSpeed > limitSpeed)) { // adjust speed 'till it breaks	
				//Serial.print(" Try at: "); Serial.println(i2C_Talk().getI2CFrequency(), DEC); Serial.flush();
				hasFailed = recovery().testDevice(2, 1);
				if (hasFailed) {
					limitSpeed = bestSpeed - adjustBy / 100;
					//Serial.print("\n Failed. NewLimit:"); Serial.println(limitSpeed, DEC); Serial.flush();
				}
				else {
					//Serial.print(" OK. BestSpeed for 0x"); Serial.print(foundDeviceAddr, HEX); Serial.print(" was:"); Serial.println(bestSpeed, DEC); Serial.flush();
					bestSpeed = i2C_Talk().setI2CFrequency(bestSpeed + adjustBy);
				}
			}
			bestSpeed = i2C_Talk().setI2CFrequency(bestSpeed - adjustBy);
			hasFailed = 0;
			adjustBy /= 2;
		} while (abs(adjustBy) > bestSpeed/5);
	}
	hasFailed = adjustSpeedTillItWorksAgain(i2c_device, (adjustBy > 0 ? -10 : 10));
	bestSpeed = i2C_Talk().getI2CFrequency();
	return hasFailed ? _I2C_Device_Not_Found : _OK;
}

signed char I2C_SpeedTest::adjustSpeedTillItWorksAgain(I_I2Cdevice & i2c_device, int32_t incrementRatio) {
	signed char hasFailed; // success == 0
	//Serial.print("\n ** Adjust I2C_Speed: "); Serial.println(i2C_Talk().getI2CFrequency(),DEC);Serial.flush();
	do { // Adjust speed 'till it works reliably
		hasFailed = recovery().testDevice(NO_OF_TESTS_MUST_PASS, 1);
		if (hasFailed) {
			auto increment = i2C_Talk().getI2CFrequency() / incrementRatio;
			i2C_Talk().setI2CFrequency(i2C_Talk().getI2CFrequency() + increment > 2000 ? increment : 2000);
			//Serial.print(" Adjust I2C_Speed: "); Serial.print(getI2CFrequency(),DEC); Serial.print(" increment :"); Serial.println(getI2CFrequency() / incrementRatio,DEC);
		}
	} while (hasFailed && i2C_Talk().getI2CFrequency() > I2C_Talk::MIN_I2C_FREQ && i2C_Talk().getI2CFrequency() < I2C_Talk::MAX_I2C_FREQ);
	//Serial.print(" Adjust I2C_Speed "); Serial.print(hasFailed ? "failed at : " : "finished at : "); Serial.println(i2C_Talk().getI2CFrequency(),DEC);Serial.flush();
	return hasFailed ? _I2C_Device_Not_Found : _OK;
}

