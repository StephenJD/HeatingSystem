#pragma once
#include <I2C_Scan.h>
#include <I2C_Talk.h>
#include <Arduino.h>

class I_I2Cdevice;

class I2C_SpeedTest {
public:
	template<bool non_stop, bool serial_out>
	uint32_t speedTest_T(I_I2Cdevice * deviceFailTest = 0);
	
	uint32_t speedTest(I_I2Cdevice * deviceFailTest = 0);

	void setI2C_Address(int16_t devAddr) { _devAddr = devAddr; }
	bool isInScanOrSpeedTest(){ return _inScanOrSpeedTest; }
	signed char adjustSpeedTillItWorksAgain(I_I2Cdevice * deviceFailTest, int32_t increment);
	void reset();
	void prepareNextTest();

	int32_t thisHighestFreq = 0;
	//int32_t thisLowestFreq = 0;

	int32_t maxSafeSpeed = 0;
	//int32_t minSafeSpeed = 0;

private:
	signed char testDevice(I_I2Cdevice * deviceFailTest, int16_t addr, int noOfTestsMustPass);
	signed char findOptimumSpeed(I_I2Cdevice * deviceFailTest, int32_t & bestSpeed, int32_t limitSpeed);
	bool _inScanOrSpeedTest = false;

	I2C_Talk * _i2C;

	int32_t _i2cFreq;
	int32_t _lastGoodi2cFreq;
	unsigned long _lastRestartTime;
	const uint8_t _I2C_DATA_PIN = 20;
	int16_t _devAddr;
	uint8_t	error = 0;

};

template<> // specialization implemented in .cpp
uint32_t I2C_SpeedTest::speedTest_T<false,false>(I_I2Cdevice * deviceFailTest);

inline uint32_t I2C_SpeedTest::speedTest(I_I2Cdevice * deviceFailTest) { return speedTest_T<false, false>(deviceFailTest); }

template<bool non_stop, bool serial_out>
uint32_t I2C_SpeedTest::speedTest_T(I_I2Cdevice * deviceFailTest) {
	auto scanner = I2C_Scan(_i2C);
	if (serial_out) {
		if (non_stop) {
			Serial.println("Start Speed Test...");
			scanner.show_next();
			setI2C_Address(scanner.foundDeviceAddr);
		} else {
			if (_devAddr > 127) {
				Serial.print("Device address out of range: 0x");
			}
			else {
				Serial.print("\nTest Device at: 0x");
			}
			Serial.println(_devAddr,HEX);
		}
	}

	do  {
		speedTest_T<false,false>(deviceFailTest);
	} while(non_stop && scanner.show_next() && setI2C_Address(scanner.foundDeviceAddr), true);

	if (serial_out) {
		if (non_stop) {
			Serial.print("Overall Best Frequency: "); 
			Serial.println(maxSafeSpeed); Serial.println();
			//Serial.print("Overall Min Frequency: "); 
			//Serial.println(minSafeSpeed); Serial.println();
		} else {
			if (error == I2C_Talk::_OK) {
				Serial.print(" Final Max Frequency: "); Serial.println(thisHighestFreq);
				//Serial.print(" Final Min Frequency: "); Serial.println(thisLowestFreq); Serial.println();
			} else {
				Serial.println(" Test Failed");
				Serial.println(I2C_Talk::getError(error));
				Serial.println();
			}
		}
	}

	return non_stop ? maxSafeSpeed : thisHighestFreq;
}
