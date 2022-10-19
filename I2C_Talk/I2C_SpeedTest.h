#pragma once
#include <I2C_Scan.h>
#include <I2C_Talk.h>
#include <I2C_Talk_ErrorCodes.h>
#include <Arduino.h>
#include <I2C_Device.h>

#ifdef DEBUG_SPEED_TEST
#include <Logging.h>
#endif

/// <summary>
/// <para>Supports Scanning and Speed-Testing</para>
/// Usage:
///	<para>auto i2C = I2C_Talk{};</para>
/// <para>I2C_Recover i2c_recover{ i2C };</para>
/// <para>I_I2Cdevice_Recovery my_device{ i2c_recover, addr };</para>
///	<para>auto speedTest = I2C_SpeedTest(my_device);</para>
///	<para>speedTest.show_fastest()   // Return max speed for this address using status()</para>
/// <para>speedTest.fastest(my_device) // Return max speed for this address using device test function</para>
///	
///	or to LCD ...
///	<para>speedTest.prepareNextTest(); // reset to start scanning at 0</para>
///	<para>while (scanner.next()){ // return after each device found. Don't output to serial port.</para>
///	<para>	lcd->print(scanner.foundDeviceAddr,HEX);</para>
///	<para>}</para>
///	</summary>
class I2C_SpeedTest {
public:
	I2C_SpeedTest() = default;
	I2C_SpeedTest(I_I2Cdevice_Recovery & i2c_device) : _i2c_device(&i2c_device) {
#ifdef DEBUG_SPEED_TEST		
		arduino_logger::logger() << F("  I2C_SpeedTest device 0x") << arduino_logger::L_hex << i2c_device.getAddress() << arduino_logger::L_endl;
#endif
	}
	static bool doingSpeedTest() { return _is_inSpeedTest; }
	auto error() const -> I2C_Talk_ErrorCodes::Error_codes{ return _error; }
	int32_t thisHighestFreq() const { return _thisHighestFreq; }

	uint32_t fastest();
	uint32_t fastest(I_I2Cdevice_Recovery & i2c_device);
	uint32_t show_fastest();
	uint32_t show_fastest(I_I2Cdevice_Recovery & i2c_device);
	void prepareNextTest();
	//const __FlashStringHelper * getStatusMsg() const { return I2C_Talk::getStatusMsg(error()); }
	static constexpr int NO_OF_TESTS_MUST_PASS = 5;
private:
	template<bool serial_out>
	uint32_t fastest_T();

	auto adjustSpeedTillItWorksAgain(int32_t increment)-> I2C_Talk_ErrorCodes::Error_codes;
	auto testDevice(int noOfTests, int allowableFailures)->I2C_Talk_ErrorCodes::Error_codes;
	auto findOptimumSpeed(int32_t & bestSpeed, int32_t limitSpeed)->I2C_Talk_ErrorCodes::Error_codes;
	
	static bool _is_inSpeedTest;
	I_I2Cdevice_Recovery * _i2c_device = 0;
	int32_t _thisHighestFreq = 0;
	I2C_Talk_ErrorCodes::Error_codes _error = I2C_Talk_ErrorCodes::_OK;
};

// specialization implemented in .cpp
template<> 
uint32_t I2C_SpeedTest::fastest_T<false>();

inline uint32_t I2C_SpeedTest::fastest() {return fastest_T<false>(); }
inline uint32_t I2C_SpeedTest::show_fastest() { return fastest_T<true>(); }

template<bool serial_out>
uint32_t I2C_SpeedTest::fastest_T() {
		
	if /*consexpr*/ (serial_out) {
			Serial.print("\nTest Device at: 0x");
			Serial.println(_i2c_device->getAddress(),HEX);
	}
		
	fastest_T<false>();
	if  /*consexpr*/ (serial_out) {
		if (error() == I2C_Talk_ErrorCodes::_OK) {
			Serial.print(" Final Max Frequency: "); Serial.println(thisHighestFreq());
		}
		else {
			Serial.println(" Test Failed");
			Serial.println(I2C_Talk::getStatusMsg(error()));
			Serial.println();
		}
	}
	
	return thisHighestFreq();
}

///////////////////////  SpeedTestAll ///////////////////////////////

class I_I2C_SpeedTestAll : public I2C_Scan {
public:
	I_I2C_SpeedTestAll(I2C_Talk & i2c, I2C_Recovery::I2C_Recover & recover = I2C_Scan::nullRecovery);
	int8_t prepareTestAll(); // returns addr 0;
	uint32_t fastest(uint8_t devAddr);
	uint32_t show_fastest(uint8_t devAddr);
	uint32_t show_fastest(I_I2Cdevice_Recovery & i2c_device);
	uint32_t show_fastest() { return show_fastest(scanner().foundDeviceAddr); }
	uint32_t showAll_fastest();
	int32_t thisHighestFreq() const { return _speedTester.thisHighestFreq(); }
	int32_t maxSafeSpeed() const { return _maxSafeSpeed; }
	uint8_t error() const { return _speedTester.error(); }

protected:
	I2C_Scan & scanner() { return *this; }
	I_I2Cdevice_Recovery & scan_device() {return device();}
private:
	template<bool non_stop, bool serial_out>
	uint32_t fastest_T();

	int32_t _maxSafeSpeed = 0;
	I2C_SpeedTest _speedTester;
};

inline uint32_t I_I2C_SpeedTestAll::fastest(uint8_t devAddr) { scan_device().setAddress(devAddr); return fastest_T<false, false>(); }

inline uint32_t I_I2C_SpeedTestAll::show_fastest(uint8_t devAddr) { scan_device().setAddress(devAddr); return fastest_T<false, true>(); }
inline uint32_t I_I2C_SpeedTestAll::show_fastest(I_I2Cdevice_Recovery & i2c_device) { _speedTester = i2c_device; return _speedTester.show_fastest(); }
inline uint32_t I_I2C_SpeedTestAll::showAll_fastest() { scan_device().setAddress(0); return fastest_T<true, true>(); }

template<bool non_stop, bool serial_out>
uint32_t I_I2C_SpeedTestAll::fastest_T() {
		
	// Lambda
	auto testNext = [this ]() {
		if /*consexpr*/ (non_stop) {
			if (scanner().next_T<false, serial_out>()) {
				scan_device().setAddress(scanner().foundDeviceAddr);
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
			Serial.println(scan_device().getAddress(),HEX);
		}
	}
	if /*consexpr*/ (non_stop) {
		prepareTestAll();
		scanner().next_T<false,serial_out>();
		scan_device().setAddress(scanner().foundDeviceAddr);
	}

	do {
		_speedTester.fastest(scan_device());
		if (_speedTester.thisHighestFreq() < maxSafeSpeed()) _maxSafeSpeed = _speedTester.thisHighestFreq();
		if  /*consexpr*/ (serial_out) {
			if (_speedTester.error() == I2C_Talk_ErrorCodes::_OK) {
				Serial.print(" Final Max Frequency: "); Serial.println(_speedTester.thisHighestFreq());
			}
			else if /*consexpr*/ (!non_stop) {
				Serial.println(" Test Failed");
				Serial.println(I2C_Talk::getStatusMsg(_speedTester.error()));
				Serial.println();
			}
		}
	} while (testNext());

	if /*consexpr*/ (serial_out && non_stop) {
		Serial.print("Number of Devices: "); Serial.println(scanner().totalDevicesFound);
		Serial.print("Overall Best Frequency: ");
		Serial.println(maxSafeSpeed()); Serial.println();
	}
	return non_stop ? maxSafeSpeed() : _speedTester.thisHighestFreq();
}
