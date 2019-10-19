#include <Arduino.h>
#include <I2C_Talk_ErrorCodes.h>
#include <I2C_Talk.h>
#include <i2c_Device.h>
#include <I2C_RecoverRetest.h>

#include <Clock.h>
#include <Logging.h>
#include <EEPROM.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <Conversions.h>
#include <Date_Time.h>

#define I2C_RESET 14
#define RTC_RESET 4
#define RTC_ADDRESS 0x68

int st_index;
using namespace I2C_Recovery;

const uint8_t DS75LX_Temp = 0x00; // two bytes must be read. Temp is MS 9 bits, in 0.5 deg increments, with MSB indicating -ve temp.
const uint8_t DS75LX_Config = 0x01;
const uint8_t DS75LX_HYST_REG = 0x02;
const uint8_t DS75LX_LIMIT_REG = 0x03;
const uint8_t EEPROM_CLOCK_ADDR = 0;

I2C_Talk rtc{ Wire1 };

EEPROMClass & eeprom() {
	static EEPROMClass_T<rtc> _eeprom_obj{ 0x50 };
	return _eeprom_obj;
}

EEPROMClass & EEPROM = eeprom();

Clock & clock_() {
	//	static Clock _clock;
	static Clock_I2C<rtc> _clock(RTC_ADDRESS);
	return _clock;
}

Logger & logger() {
	static Serial_Logger _log(9600, clock_());
	return _log;
}

class I2C_Temp_Sensor : public I_I2Cdevice_Recovery {
public:
	I2C_Temp_Sensor(I2C_Recovery::I2C_Recover & recover, int addr) : I_I2Cdevice_Recovery(recover, addr) {}
	I2C_Temp_Sensor() = default; // allow array to be constructed

	// Queries
	int8_t get_temp() const {return (get_fractional_temp() + 128) / 256;}
	int16_t get_fractional_temp() const { return lastGood;	}

	bool hasError() { return _error != I2C_Talk_ErrorCodes::_OK; }

	// Modifiers
	int8_t readTemperature() {
		uint8_t temp[2];
		_error = read(DS75LX_Temp, 2, temp);
		lastGood = (temp[0] << 8) | temp[1];
		return _error;
	}

	uint8_t setHighRes() {
		_error = write(DS75LX_Config, 0x60);
		return _error;
	}

	// Virtual Functions
	uint8_t testDevice() override {
		uint8_t temp[2] = { 75,0 };
		Serial.print("Temp_Sensor::testDevice : "); Serial.println(getAddress());
		return write_verify(DS75LX_HYST_REG, 2, temp);
	}

	void initialise(int address) {
		setAddress(address);
		readTemperature();
	}

protected:
	int _error = 0;;
private:
	int16_t lastGood = 0;
};

I2C_Talk i2C;
I2C_Recover_Retest recover_retest{};
I_I2C_SpeedTestAll i2c_test{i2C, recover_retest };

I2C_Temp_Sensor * tempSens = 0;
I_I2C_SpeedTestAll rtc_test{rtc};

uint8_t resetI2c(I2C_Talk & i2c, int) {
  Serial.println("Power-Down I2C...");
  digitalWrite(I2C_RESET, LOW);
  delayMicroseconds(5000);
  digitalWrite(I2C_RESET, HIGH);
  delayMicroseconds(5000);
  i2c.restart();
  return I2C_Talk_ErrorCodes::_OK;
}

uint8_t resetRTC(I2C_Talk & i2c, int addr) {
	Serial.println("Power-Down RTC...");
	digitalWrite(RTC_RESET, HIGH);
	delayMicroseconds(200000);
	digitalWrite(RTC_RESET, LOW);
	i2c.restart();
	delayMicroseconds(200000);
	return true;
}

bool checkEEPROM(const char * msg) {
	auto functionReturnTime = micros();
	auto loopTime = functionReturnTime + 1000;
	auto timeNow = functionReturnTime;
	bool OK = true;
	//int loopCount = 1;
	while (timeNow < loopTime) {
		//if (EEPROM.getStatus() != 0) {
		if (rtc.status(0x50) != 0) {
			resetRTC(rtc, 0x50);
			logger().log("\nEEPROM error after check uS: ", timeNow - functionReturnTime, msg);
			OK = false;
			break;
		}
		timeNow = micros();
		//++loopCount;
	}
	return OK;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600);
	Serial.println(" Serial Begun");
	pinMode(I2C_RESET, OUTPUT);
	pinMode(RTC_RESET, OUTPUT);
	resetI2c(i2C, 0);
	resetRTC(rtc, 0);

	Serial.print("Wire addr   "); Serial.println((long)&Wire);
	Serial.print("Wire 1 addr "); Serial.println((long)&Wire1);

	rtc_test.showAll_fastest();

	recover_retest.setTimeoutFn(resetI2c);
	
	tempSens = new I2C_Temp_Sensor(recover_retest, 0x29);

	Serial.print("TS: "); Serial.println(tempSens->get_temp());

	Serial.println(" Speedtest All Addr");
	i2c_test.showAll_fastest();
	tempSens->readTemperature();
	Serial.print("TS: "); Serial.println(tempSens->get_temp());

	Serial.println(" Speedtest Each Adr");
	auto addr = i2c_test.prepareTestAll();
	while (addr = i2c_test.next()) {
		i2c_test.show_fastest();
	}
	tempSens->readTemperature();
	Serial.print("TS: "); Serial.println(tempSens->get_temp());

	Serial.println(" Speedtest Each Device");
	addr = i2c_test.prepareTestAll();
	while (addr = i2c_test.next()) {
		tempSens->setAddress(addr);
		i2c_test.show_fastest(*tempSens);
	}
	tempSens->readTemperature();
	Serial.print("TS: "); Serial.println(tempSens->get_temp());
}

void loop() {
	tempSens->readTemperature();
	Serial.print("TS: "); Serial.println(tempSens->get_temp());
	delay(5000);
}
