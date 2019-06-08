#include <Arduino.h>
#include <I2C_Talk_ErrorCodes.h>
#include <I2C_Talk.h>
#include <i2c_Device.h>
#include <I2C_Scan.h>
#include <I2C_RecoverRetest.h>

#include <Clock.h>
#include <Logging.h>
#include <Wire.h>

#define I2C_RESET 14
#define RTC_RESET 4
constexpr uint8_t TEMP_HYST_REG = 2;
constexpr uint8_t TEMP_LIMIT_REG = 3;

Clock & clock_() {
	static Clock _clock;
	return _clock;
}

Logger & logger() {
	static Serial_Logger _log(9600, clock_());
	return _log;
}

I2C_Talk rtc(Wire1);
I2C_Talk i2C;
auto i2C_Test = I2C_Recover_Retest{i2C};

uint8_t resetI2c(I2C_Talk & i2c, int) {
	Serial.println("Power-Down I2C...");
	digitalWrite(I2C_RESET, LOW);
	delayMicroseconds(5000);
	digitalWrite(I2C_RESET, HIGH);
	delayMicroseconds(5000);
	i2c.restart();
	return 0;
}

class I2c_eeprom : public I_I2Cdevice {
public:
	using I_I2Cdevice::I_I2Cdevice;
	uint8_t testDevice() override {
		uint8_t dataBuffa[3];
		uint8_t hasFailed = readEP(0, sizeof(dataBuffa), dataBuffa);
		//Serial.print("Test I2C_EEPROM at : 0x"); Serial.print(addr, HEX); Serial.print(" Read gave :"); Serial.print(i2c.getStatusMsg(hasFailed));
		if (!hasFailed) hasFailed = writeEP(0, sizeof(dataBuffa), dataBuffa);
		//Serial.print(" Write gave :"); Serial.println(i2c.getStatusMsg(hasFailed));
		return hasFailed;
	}
};

class I2c_rtc : public I_I2Cdevice {
public:
	using I_I2Cdevice::I_I2Cdevice;
	uint8_t testDevice() override {
		//Serial.println("Test RTC.");
		uint8_t data[9];
		return read(0, 9, data);
	}
};

class I2C_TempSensor : public I2Cdevice {
public:
	using I2Cdevice::I2Cdevice;
	uint8_t testDevice() override {
		Serial.print("Test I2C_TempSensor at 0x"); Serial.println(getAddress(), HEX);
		uint8_t dataBuffa[2] = { 75,0 };
		return write_verify(2, 2, dataBuffa);
	}
};

I2C_TempSensor tempSens[27] = {i2C_Test};

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600);
	Serial.println("Serial Begun");
	pinMode(I2C_RESET, OUTPUT);
	digitalWrite(I2C_RESET, HIGH); // reset pin
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	i2C.restart();
	i2C_Test.setTimeoutFn(resetI2c);
	//i2C_Test.showAll_fastest();

	Serial.println("\nSpeedtest Each addr\n");
	int tsIndex = 0;
	i2C_Test.foundDeviceAddr = 0x28;
	while (i2C_Test.next()) {
		Serial.print("\nSpeed Test for 0x"); Serial.print(i2C_Test.foundDeviceAddr, HEX);
		tempSens[tsIndex].setAddress(i2C_Test.foundDeviceAddr);

		tempSens[tsIndex].set_runSpeed(i2C_Test.show_fastest()); 
		Serial.print("Speed set for 0x"); Serial.flush();
		auto addr = tempSens[tsIndex].getAddress();
		Serial.print(addr, HEX); Serial.flush();
		auto speed = tempSens[tsIndex].runSpeed();
		Serial.print(" to: "); Serial.println(tempSens[tsIndex].runSpeed()); Serial.flush();
		++tsIndex;
		break;
	}

	Serial.println("\nOperational Speeds I2C:");
	for (auto & ts : tempSens) {
		auto addr = ts.getAddress();
		if (addr == 0) break;
		Serial.print(" Speed of : 0x"); Serial.print(addr, HEX); Serial.print(" is :"); Serial.println(ts.runSpeed(), DEC);
	}

	Serial.println("\nRetest with provided test funtions...");

	Serial.println("\nSpeedtest I2C\n");
	for (auto & ts : tempSens) {
		if (ts.getAddress() == 0) break;
		i2C_Test.show_fastest(ts);
	}

	Serial.println("\nPost-Test Operational Speeds I2C:");
	for (auto & ts : tempSens) {
		auto addr = ts.getAddress();
		if (addr == 0) break;
		Serial.print(" Speed of : 0x"); Serial.print(addr, HEX); Serial.print(" is :"); Serial.println(ts.runSpeed(), DEC);
	}

	char dataBuffa[] = {
  "Twas brillig, and the slithy toves \nDid gyre and gimble in the wabe;\nAll mimsy were the borogoves,\
\nAnd the mome raths outgrabe. \n\nBeware the Jabberwock, my son! \nThe jaws that bite, the claws that catch!\
\nBeware the Jubjub bird, and shun \nThe frumious Bandersnatch!\
\n\nHe took his vorpal sword in hand: \nLong time the manxome foe he sought -\nSo rested he by the Tumtum tree,\
\nand stood awhile in thought.\n\nAnd as in uffish thought he stood, \nThe Jabberwock, with eyes of flame,\
\nCame whiffling through the tulgey wood,\nAnd burbled as it came!\n" };
	Serial.println("Write I2C_EEPROM :");
	Serial.println(dataBuffa);
	uint8_t hasFailed = rtc.writeEP(0x50, 0, sizeof(dataBuffa), dataBuffa);

	for (char & chr : dataBuffa) { chr = 0; }

	Serial.println("\nRead I2C_EEPROM :");
	if (!hasFailed) hasFailed = rtc.readEP(0x50, 0, sizeof(dataBuffa), dataBuffa);
	Serial.println(dataBuffa);
	Serial.println("\nFinished");
}

void loop() {

}