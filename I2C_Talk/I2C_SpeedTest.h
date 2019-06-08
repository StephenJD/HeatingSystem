#pragma once
#include <I2C_Scan.h>
#include <I2C_Talk.h>
#include <I2C_Talk_ErrorCodes.h>
#include <Arduino.h>
#include <I2C_Device.h>

/// <summary>
/// <para>Supports Scanning and Speed-Testing</para>
/// Usage:
///	<para>auto i2C = I2C_Talk{};</para>
/// <para>auto device = I2C_Device(i2C, 0x41);</para>
///	<para>auto speedTest = I2C_SpeedTest(i2C);</para>
///	<para>speedTest.show_fastest(0x41)   // Return max speed for this address using notExists()</para>
/// <para>speedTest.fastest(device) // Return max speed for this address using device test function</para>
///	
///	or to LCD ...
///	<para>speedTest.prepareNextTest(); // reset to start scanning at 0</para>
///	<para>while (scanner.next()){ // return after each device found. Don't output to serial port.</para>
///	<para>	lcd->print(scanner.foundDeviceAddr,HEX); lcd->print(" ");</para>
///	<para>}</para>
///	</summary>
class I2C_SpeedTest : public I2C_Scan {
public:
	static constexpr int NO_OF_TESTS_MUST_PASS = 5;

	I2C_SpeedTest(I2C_Talk & i2C) : I2C_Scan(i2C) {}
	I2C_SpeedTest(I2C_Talk & i2C, I2C_Recover & recover) : I2C_Scan(i2C, recover) {}
	
	uint32_t fastest(I_I2Cdevice & i2c_device);
	uint32_t fastest(uint8_t devAddr);

	uint32_t show_fastest(I_I2Cdevice & i2c_device);
	uint32_t show_fastest(uint8_t devAddr);
	uint32_t show_fastest() { return show_fastest(foundDeviceAddr); }
	uint32_t showAll_fastest();

	signed char adjustSpeedTillItWorksAgain(I_I2Cdevice & i2c_device, int32_t increment);
	int8_t prepareTestAll(); // returns addr 0;
	void prepareNextTest();
	const char * getStatusMsg() { return I2C_Talk::getStatusMsg(error); }

	int32_t thisHighestFreq = 0;

	int32_t maxSafeSpeed = 0;
protected:
	I2C_SpeedTest() = default;
private:
	template<bool non_stop, bool serial_out>
	uint32_t fastest_T(I_I2Cdevice & i2c_device);
	
	uint8_t testDevice(int noOfTests, int maxNoOfFailures) { return recovery().testDevice(noOfTests, maxNoOfFailures); }
	signed char findOptimumSpeed(I_I2Cdevice & i2c_device, int32_t & bestSpeed, int32_t limitSpeed);
	unsigned long _lastRestartTime;
	int32_t _i2cFreq;
	int32_t _lastGoodi2cFreq;
	uint8_t	error = 0;

	const uint8_t _I2C_DATA_PIN = 20;
};

template<> // specialization implemented in .cpp
uint32_t I2C_SpeedTest::fastest_T<false,false>(I_I2Cdevice & i2c_device);

inline uint32_t I2C_SpeedTest::fastest(I_I2Cdevice & i2c_device) {return fastest_T<false, false>(i2c_device); }
inline uint32_t I2C_SpeedTest::fastest(uint8_t devAddr) { auto device = I_I2Cdevice(i2C_Talk(), devAddr); return fastest_T<false, false>(device); }

inline uint32_t I2C_SpeedTest::show_fastest(I_I2Cdevice & i2c_device) { return fastest_T<false, true>(i2c_device); }
inline uint32_t I2C_SpeedTest::show_fastest(uint8_t devAddr) { auto device = I_I2Cdevice(i2C_Talk(), devAddr); return fastest_T<false, true>(device); }
inline uint32_t I2C_SpeedTest::showAll_fastest() { auto device = I_I2Cdevice(i2C_Talk(), 0); return fastest_T<true, true>(device); }


template<bool non_stop, bool serial_out>
uint32_t I2C_SpeedTest::fastest_T(I_I2Cdevice & i2c_device) {
		
	// Lambda
	auto testNext = [&i2c_device, this ]() {
		if /*consexpr*/ (non_stop) {
			if (next_T<false, serial_out>()) {
				i2c_device.setAddress(foundDeviceAddr);
				return true;
			}
		}
		return false;
	};

	// Algorithm
	if /*consexpr*/ (serial_out) {
		if /*consexpr*/ (non_stop) {
			Serial.println("\nStart Speed Test...");
		} else {
			Serial.print("\nTest Device at: 0x");
			Serial.println(i2c_device.getAddress(),HEX);
		}
	}
	if /*consexpr*/ (non_stop) {
		prepareTestAll();
		next_T<false,serial_out>();
		i2c_device.setAddress(foundDeviceAddr);
	}

	do {
		fastest_T<false, false>(i2c_device);
		if  /*consexpr*/ (serial_out) {
			if (error == I2C_Talk_ErrorCodes::_OK) {
				Serial.print(" Final Max Frequency: "); Serial.println(thisHighestFreq);
				//Serial.print(" Final Min Frequency: "); Serial.println(thisLowestFreq); Serial.println();
			}
			else if /*consexpr*/ (!non_stop) {
				Serial.println(" Test Failed");
				Serial.println(I2C_Talk::getStatusMsg(error));
				Serial.println();
			}
		}
	} while (testNext());

	if /*consexpr*/ (serial_out && non_stop) {
		Serial.print("Number of Devices: "); Serial.println(totalDevicesFound);
		Serial.print("Overall Best Frequency: ");
		Serial.println(maxSafeSpeed); Serial.println();
		//Serial.print("Overall Min Frequency: "); 
		//Serial.println(minSafeSpeed); Serial.println();
	}
	return non_stop ? maxSafeSpeed : thisHighestFreq;
}
