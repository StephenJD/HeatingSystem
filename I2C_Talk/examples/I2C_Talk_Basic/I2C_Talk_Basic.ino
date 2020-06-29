#include <Arduino.h>
#include <I2C_Talk.h>
#include "I2C_Recover.h"
#include <I2C_Scan.h>
#include <I2C_SpeedTest.h>

#include <Logging.h>
#include <EEPROM.h>

#define I2C_RESET 14
#define SERIAL_RATE 115200

//////////////////////////////// Start execution here ///////////////////////////////
using namespace I2C_Recovery;

#if defined(__SAM3X8E__)
	I2C_Talk rtc{ Wire1 }; // not initialised until this translation unit initialised.
	EEPROMClass & eeprom() {
		static EEPROMClass_T<rtc> _eeprom_obj{ (rtc.ini(Wire1), 0x50) }; // rtc will be referenced by the compiler, but rtc may not be constructed yet.
		return _eeprom_obj;
	}
#else
	EEPROMClass & eeprom() {
		return EEPROM;
}
#endif

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	return _log;
}

I2C_Talk i2C;
//I2C_Recover i2c_recover(i2C);

I_I2C_Scan scanner{i2C};
I2C_SpeedTest speedTest{};
I_I2C_SpeedTestAll speedTestAll{i2C};

void setup() {
	Serial.begin(SERIAL_RATE);
	Serial.println("Serial Begun"); Serial.flush();
	pinMode(I2C_RESET, OUTPUT);
	digitalWrite(I2C_RESET, HIGH); // reset pin
	i2C.restart();

	scanner.scanFromZero();
	auto result = scanner.show_next();
	Serial.print(" Show-next. Error:"); Serial.println(scanner.getStatusMsg());
	Serial.print(" Show-next. Result:"); Serial.println(result, HEX);
	Serial.print(" Show-next. Addr:"); Serial.println(scanner.foundDeviceAddr, HEX);
	Serial.print(" Show-next. Count:"); Serial.println(scanner.totalDevicesFound);
	Serial.println(); Serial.flush();

	result = scanner.show_next();
	Serial.print(" Show-next again. Error:"); Serial.println(scanner.getStatusMsg());
	Serial.print(" Show-next again. Result:"); Serial.println(result, HEX);
	Serial.print(" Show-next again. Addr:"); Serial.println(scanner.foundDeviceAddr, HEX);
	Serial.print(" Show-next again. Count:"); Serial.println(scanner.totalDevicesFound);
	Serial.println(); Serial.flush();

	scanner.scanFromZero();
	result = scanner.next();
	Serial.println();
	Serial.print(" next. Error:"); Serial.println(scanner.getStatusMsg());
	Serial.print(" next. Result:"); Serial.println(result, HEX);
	Serial.print(" next. Addr:"); Serial.println(scanner.foundDeviceAddr, HEX);
	Serial.print(" next. Count:"); Serial.println(scanner.totalDevicesFound);
	Serial.println(); Serial.flush();
	auto addr = scanner.foundDeviceAddr;

	result = scanner.next();
	Serial.println();
	Serial.print(" next again. Error:"); Serial.println(scanner.getStatusMsg());
	Serial.print(" next again. Result:"); Serial.println(result, HEX);
	Serial.print(" next again. Addr:"); Serial.println(scanner.foundDeviceAddr, HEX);
	Serial.print(" next again. Count:"); Serial.println(scanner.totalDevicesFound);
	Serial.println(); Serial.flush();

	speedTestAll.scanFromZero();
	Serial.print(" speedTestAll Addr:"); Serial.println(speedTestAll.foundDeviceAddr, HEX);

	addr = speedTestAll.show_next();
	auto speed = speedTestAll.show_fastest(addr);
	Serial.println();
	Serial.print(" show_fastest. Error:"); Serial.println(speedTestAll.getStatusMsg());
	Serial.print(" show_fastest. Speed:"); Serial.println(speed);
	Serial.println(); Serial.flush();
}

void loop() {
	auto result = scanner.show_all();
	Serial.println(); Serial.flush();

	delay(1000);
}