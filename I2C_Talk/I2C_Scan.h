#pragma once
#include <Arduino.h>
#include <I2C_Talk_ErrorCodes.h>
#include <I2C_Talk.h>
#include <I2C_Device.h>

/// <summary>
/// Usage:
///	<para>auto i2C = I2C_Talk{};</para>
///	<para>auto scanner = I2C_Scan(i2C);</para>
///	
///	<para>scanner.show_all()// scan all addresses and print results to serial port</para>
///	<para>scanner.show_next()// scan next address and print results to serial port</para>
///	or to LCD ...
///	<para>scanner.scanFromZero(); // reset to start scanning at 0</para>
///	<para>while (scanner.next()){ // return after each device found. Don't output to serial port.</para>
///	<para>	lcd->print(scanner.foundDeviceAddr,HEX);</para>
///	<para>}</para>
///	</summary>
class I2C_Scan {
public:	
	I2C_Scan(I2C_Talk & i2c, I2C_Recovery::I2C_Recover & recovery = I2C_Scan::nullRecovery);
	void scanFromZero();
	int8_t next() { return next_T<false, false>(); } // returns next address
	int8_t show_all() { return next_T<true, true>(); } // returns last address
	int8_t show_next() { return next_T<false, true>(); } // returns next address
	//const __FlashStringHelper * getStatusMsg() { return I2C_Talk::getStatusMsg(error()); }
	bool isInScanOrSpeedTest() { return _inScanOrSpeedTest; } // to disable post-reset ini during scan/speed tesrt
	I_I2Cdevice_Recovery & device() { return _device; }
	uint8_t error() const { return _error; }

	int8_t foundDeviceAddr = -1;	// -1 to 127
	uint8_t totalDevicesFound = 0;	
	static I2C_Recovery::I2C_Recover nullRecovery;
	I2C_Talk & i2C() { return _device.i2C(); }
protected:
	I2C_Recovery::I2C_Recover & recovery() { return device().recovery(); }
	void inScan(bool isInScan) { _inScanOrSpeedTest = isInScan; }
	uint8_t	_error = 0;
private:
	friend class I_I2C_SpeedTestAll;
	template<bool non_stop, bool serial_out>
	int8_t next_T();
	
	int8_t nextDevice(); // returns address of next device found or zero

	I_I2Cdevice_Recovery _device;
	bool _inScanOrSpeedTest = false;
};

//*************************************************************************************
// Template Function implementations //
template<bool non_stop, bool serial_out>
int8_t I2C_Scan::next_T(){
	if /*constexpr*/ (non_stop) {
		scanFromZero();
		if /*constexpr*/(serial_out) Serial.println("\nScan");
	}
	while(nextDevice()) {
		if /*constexpr*/(serial_out) {
			if (error() == I2C_Talk_ErrorCodes::_OK) {
				Serial.print("I2C Device at: 0x"); Serial.println(foundDeviceAddr,HEX);
			} else {
				Serial.print("I2C Error at: 0x"); 
				Serial.print(foundDeviceAddr, HEX);
				Serial.println(I2C_Talk::getStatusMsg(error()));
			}
		}
		if /*constexpr*/(!non_stop) break;
	}
	if /*constexpr*/(non_stop && serial_out) {
		Serial.print("Total I2C Devices: "); 
		Serial.println(totalDevicesFound,DEC);
		Serial.println();
	}
	return foundDeviceAddr;
}

