#include <Arduino.h>
#include <I2C_Talk.h>
#include "I2C_Recover.h"
#include <I2C_Scan.h>
#include <I2C_SpeedTest.h>

#include <Logging.h>
#include <TempSensor.h>

#define I2C_RESET 14
#define SERIAL_RATE 115200

//////////////////////////////// Start execution here ///////////////////////////////
using namespace I2C_Recovery;
using namespace HardwareInterfaces;

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	return _log;
}

I2C_Talk i2C;
auto i2c_recover = I2C_Recover{i2C};
I2C_Scan scanner{ i2C };
///	<para>speedTest.show_fastest(0x41)
auto anyTempSensor = TempSensor(i2c_recover);

void setup() {
	Serial.begin(SERIAL_RATE);
	Serial.println("Serial Begun"); Serial.flush();
	pinMode(I2C_RESET, OUTPUT);
	digitalWrite(I2C_RESET, HIGH); // reset pin
	i2C.begin();
	//i2C.extendTimeouts(5000, 60, 5000);
	logger() << L_allwaysFlush << "Start" << L_endl;
}

void loop() {
	scanner.scanFromZero();
	auto devAddr = scanner.show_next();
	if (devAddr == 0) logger() << F("No TS detected\n");
	while (devAddr) {

		anyTempSensor.setAddress(devAddr);
		//anyTempSensor.setAddress(0x29);
		anyTempSensor.setHighRes();
		auto speedTest = I2C_SpeedTest(anyTempSensor);
		speedTest.show_fastest();
		auto status = anyTempSensor.readTemperature();
		auto hiRes = anyTempSensor.get_fractional_temp();
		auto hiDeg = hiRes / 256;
		auto fract = hiRes & 0x00FF;
		float temp = hiDeg + (fract / 256.0);
		logger() << "TS 0x" << L_hex << anyTempSensor.getAddress() << " Temp:" << L_dec << anyTempSensor.get_temp() << " HiRes: " << temp << I2C_Talk::getStatusMsg(status) << L_endl;
		status = anyTempSensor.testDevice();
		logger() << " Test:" << I2C_Talk::getStatusMsg(status) << "\n" << L_endl;

		delay(1000);
//while (true) { ; }
		devAddr = scanner.show_next();
	}
}