#include <Arduino.h>
#include <I2C_Talk_ErrorCodes.h>
#include <I2C_Talk.h>
#include <i2c_Device.h>
#include <I2C_RecoverRetest.h>

#include <Clock.h>
#include <Logging.h>

#define I2C_RESET 14
#define RTC_RESET 4
const uint8_t DS75LX_Temp = 0x00; // two bytes must be read. Temp is MS 9 bits, in 0.5 deg increments, with MSB indicating -ve temp.
const uint8_t DS75LX_Config = 0x01;
const uint8_t DS75LX_HYST_REG = 0x02;
const uint8_t DS75LX_LIMIT_REG = 0x03;
const uint8_t EEPROM_CLOCK_ADDR = 0;


Clock & clock_() {
	static Clock _clock;
	return _clock;
}

Logger & logger() {
	static Serial_Logger _log(9600, clock_());
	return _log;
}

class I2C_Temp_Sensor : public I_I2Cdevice {
public:
	I2C_Temp_Sensor(I2C_Talk & i2C, int addr) : I_I2Cdevice(i2C, addr) {} // initialiser for first array element 
	I2C_Temp_Sensor() : I_I2Cdevice(i2c_Talk()) {} // allow array to be constructed

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
I2C_Recover_Retest recover_retest{ i2C };
I2C_Temp_Sensor * tempSens = 0;

uint8_t resetI2c(I2C_Talk & i2c, int) {
  Serial.println("Power-Down I2C...");
  digitalWrite(I2C_RESET, LOW);
  delayMicroseconds(5000);
  digitalWrite(I2C_RESET, HIGH);
  delayMicroseconds(5000);
  i2c.restart();
  return I2C_Talk_ErrorCodes::_OK;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600);
	Serial.println(" Serial Begun");
	pinMode(I2C_RESET, OUTPUT);
	digitalWrite(I2C_RESET, HIGH); // reset pin
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	i2C.restart();
	recover_retest.setTimeoutFn(resetI2c);
	
	tempSens = new I2C_Temp_Sensor(i2C, 0x29);

	Serial.print("TS: "); Serial.println(tempSens->get_temp());

	Serial.println(" Speedtest All Addr");
	recover_retest.showAll_fastest();
	tempSens->readTemperature();
	Serial.print("TS: "); Serial.println(tempSens->get_temp());

	Serial.println(" Speedtest Each Adr");
	auto addr = recover_retest.prepareTestAll();
	while (addr = recover_retest.next()) {
		recover_retest.show_fastest();
	}
	tempSens->readTemperature();
	Serial.print("TS: "); Serial.println(tempSens->get_temp());

	Serial.println(" Speedtest Each Device");
	addr = recover_retest.prepareTestAll();
	while (addr = recover_retest.next()) {
		tempSens->setAddress(addr);
		recover_retest.show_fastest(*tempSens);
	}
	tempSens->readTemperature();
	Serial.print("TS: "); Serial.println(tempSens->get_temp());
}

void loop() {
	tempSens->readTemperature();
	Serial.print("TS: "); Serial.println(tempSens->get_temp());
	delay(5000);
}
