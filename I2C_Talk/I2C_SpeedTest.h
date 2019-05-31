#pragma once

#include <Arduino.h>
class I_I2Cdevice;

class I2C_SpeedTest {
public:
	template<bool non_stop, bool serial_out>
	uint32_t speedTest_T(I_I2Cdevice * deviceFailTest = 0);

	uint32_t speedTest(I_I2Cdevice * deviceFailTest = 0) {	return speedTest_T<false, false>(deviceFailTest); }

	void setI2C_Address(int16_t devAddr) { _devAddr = devAddr; }
	bool isInScanOrSpeedTest(){ return _inScanOrSpeedTest; }
	signed char adjustSpeedTillItWorksAgain(I_I2Cdevice * deviceFailTest, int32_t increment);
	void reset();
	void prepareNextTest();

	int32_t thisHighestFreq = 0;
	int32_t thisLowestFreq = 0;

	int32_t maxSafeSpeed = 0;
	int32_t minSafeSpeed = 0;

private:
	signed char testDevice(I_I2Cdevice * deviceFailTest, uint8_t addr, int noOfTestsMustPass);
	signed char findAworkingSpeed(I_I2Cdevice * deviceFailTest);
	signed char findOptimumSpeed(I_I2Cdevice * deviceFailTest, int32_t & bestSpeed, int32_t limitSpeed);
	bool _inScanOrSpeedTest = false;

	int32_t _i2cFreq;
	int32_t _lastGoodi2cFreq;
	unsigned long _lastRestartTime;
	const uint8_t _I2C_DATA_PIN = 20;
	int16_t _devAddr;
};


template<> // specialization implemented in .cpp
uint32_t I2C_SpeedTest::speedTest_T<false,false>(I_I2Cdevice * deviceFailTest);

template<bool non_stop, bool serial_out>
uint32_t I2C_SpeedTest::speedTest_T(I_I2Cdevice * deviceFailTest) {
	if (serial_out) {
		if (non_stop) {
			Serial.println("Start Speed Test...");
			scan<false,serial_out>();
		} else {
			//if (result.foundDeviceAddr > 127) {
			//	Serial.print("Device address out of range: 0x");
			//}
			//else {
				Serial.print("\nTest Device at: 0x");
			//}
			Serial.println(result.foundDeviceAddr,HEX);
		}
	}

	do  {
		speedTest_T<false,false>(deviceFailTest);
	} while(non_stop && scan<false,serial_out>());

	if (serial_out) {
		if (non_stop) {
			Serial.print("Overall Best Frequency: "); 
			Serial.println(result.maxSafeSpeed); Serial.println();
			//Serial.print("Overall Min Frequency: "); 
			//Serial.println(result.minSafeSpeed); Serial.println();
		} else {
			if (result.error == 0) {
				Serial.print(" Final Max Frequency: "); Serial.println(result.thisHighestFreq);
				//Serial.print(" Final Min Frequency: "); Serial.println(result.thisLowestFreq); Serial.println();
			} else {
				Serial.println(" Test Failed");
				Serial.println(getError(result.error));
				Serial.println();
			}
		}
	}

	return non_stop ? result.maxSafeSpeed : result.thisHighestFreq;
}
