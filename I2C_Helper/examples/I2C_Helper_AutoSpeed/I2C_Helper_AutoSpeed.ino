#include <Logging.h>
#include <Arduino.h>
#include <Wire.h>
#include <I2C_Helper.h>

#define I2C_RESET 14
#define RTC_RESET 4
constexpr uint8_t TEMP_HYST_REG = 2;
constexpr uint8_t TEMP_LIMIT_REG = 3;

I2C_Helper_Auto_Speed<27> * i2C(0);
I2C_Helper_Auto_Speed<2> * rtc(0);

uint8_t resetI2c(I2C_Helper & i2c, int) {
	Serial.println("Power-Down I2C...");
	digitalWrite(I2C_RESET, LOW);
	delayMicroseconds(5000);
	digitalWrite(I2C_RESET, HIGH);
	delayMicroseconds(5000);
	i2c.restart();
	return i2c.result.error;
}

uint8_t resetRTC(I2C_Helper & i2c, int) {
	Serial.println("Reset RTC...");
	digitalWrite(RTC_RESET, HIGH);
	delayMicroseconds(10000);
	digitalWrite(RTC_RESET, LOW);
	delayMicroseconds(10000);
	i2c.restart();
	return i2c.result.error;
}

class I2c_eeprom : public I2C_Helper::I_I2Cdevice {
public:
	using I2C_Helper::I_I2Cdevice::I_I2Cdevice;
	uint8_t testDevice(I2C_Helper & i2c, int addr) override {
		uint8_t dataBuffa[3];
		uint8_t hasFailed = i2c.readEP(addr, 0, sizeof(dataBuffa), dataBuffa);
		//Serial.print("Test I2C_EEPROM at : 0x"); Serial.print(addr, HEX); Serial.print(" Read gave :"); Serial.print(i2c.getError(hasFailed));
		if (!hasFailed) hasFailed = i2c.writeEP(addr, 0, sizeof(dataBuffa), dataBuffa);
		//Serial.print(" Write gave :"); Serial.println(i2c.getError(hasFailed));
		return hasFailed;
	}
};

class I2c_rtc : public I2C_Helper::I_I2Cdevice {
public:
	using I2C_Helper::I_I2Cdevice::I_I2Cdevice;
	uint8_t testDevice(I2C_Helper & i2c, int addr) override {
		//Serial.println("Test RTC.");
		uint8_t data[9];
		return i2c.read(addr, 0, 9, data);
	}
};

class I2C_TempSensor : public I2C_Helper::I_I2Cdevice {
public:
	using I2C_Helper::I_I2Cdevice::I_I2Cdevice;
	uint8_t testDevice(I2C_Helper & i2c, int addr) override {
		//Serial.print("Test I2C_TempSensor at 0x"); Serial.println(addr, HEX);
		uint8_t dataBuffa[2] = { 0 };
		uint8_t hasFailed = i2c.read(addr, TEMP_HYST_REG, 2, dataBuffa);
		hasFailed |= (dataBuffa[0] != 75);
		hasFailed |= i2c.read(addr, TEMP_LIMIT_REG, 2, dataBuffa);
		hasFailed |= dataBuffa[0] != 80;
		return hasFailed;
	}
};

class I2C_TempSensorVerify : public I2C_Helper::I_I2Cdevice {
public:
	using I2C_Helper::I_I2Cdevice::I_I2Cdevice;
	uint8_t testDevice(I2C_Helper & i2c, int addr) override {
		//Serial.print("Test I2C_TempSensor at 0x"); Serial.println(addr, HEX);
		uint8_t dataBuffa[2] = { 75,0 };
		return i2c.write_verify(addr,2,2,dataBuffa);
	}
};

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600);
	Serial.println("Serial Begun");
	pinMode(I2C_RESET, OUTPUT);
	digitalWrite(I2C_RESET, HIGH); // reset pin
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin

	Serial.println(" Create i2C Helper");
	i2C = new I2C_Helper_Auto_Speed<27>(Wire, 10000);
	i2C->setTimeoutFn(resetI2c);
#if !defined NO_RTC
	Serial.println(" Create rtc Helper");
	rtc = new I2C_Helper_Auto_Speed<2>(Wire1);
	rtc->setTimeoutFn(resetRTC);

	Serial.println("\nSpeedtest RTC\n");
	rtc->result.reset();
	while (rtc->scanNextS()) {
		rtc->speedTestS();
	}
#endif

	Serial.println("\nSpeedtest I2C\n");
	i2C->result.reset();
	while (i2C->scanNextS()) {
		i2C->speedTestS();
	}

	Serial.println("\nOperational Speeds RTC:");
	for (int i = 0; i < 2; ++i) {
		auto addr = rtc->getAddress(i);
		if (addr == 0) break;
		Serial.print(" Speed of : 0x"); Serial.print(addr, HEX); Serial.print(" is :"); Serial.println(rtc->getThisI2CFrequency(addr), DEC);
	}

	Serial.println("\nOperational Speeds I2C:");
	for (int i = 0; i < 27; ++i) {
		auto addr = i2C->getAddress(i);
		if (addr == 0) break;
		Serial.print(" Speed of : 0x"); Serial.print(addr, HEX); Serial.print(" is :"); Serial.println(i2C->getThisI2CFrequency(addr), DEC);
	}

	Serial.println("\nRetest with provided test funtions...");
	I2c_eeprom myEEprom{};
	I2c_rtc myRTC{};
	I2C_TempSensor myTS{};
	I2C_TempSensorVerify myVerifyTS{};

#if !defined NO_RTC
	Serial.println("\nSpeedtest RTC\n");
	rtc->result.reset();
	I2C_Helper::I_I2Cdevice * myDevice = 0;
	while (rtc->scanNextS()) {
		if (rtc->result.foundDeviceAddr == 0x50) myDevice = &myEEprom;
		else if (rtc->result.foundDeviceAddr == 0x68) myDevice = &myRTC;
		rtc->speedTestS(myDevice);
	}
#endif

	Serial.println("\nSpeedtest I2C\n");
	i2C->result.reset();
	while (i2C->scanNextS()) {
		i2C->speedTestS(&myTS);
	}

	Serial.println("\nOperational Speeds RTC:");
	for (int i = 0; i < 2; ++i) {
		auto addr = rtc->getAddress(i);
		if (addr == 0) break;
		Serial.print(" Speed of : 0x"); Serial.print(addr, HEX); Serial.print(" is :"); Serial.println(rtc->getThisI2CFrequency(addr), DEC);
	}

	Serial.println("\nOperational Speeds I2C:");
	for (int i = 0; i < 27; ++i) {
		auto addr = i2C->getAddress(i);
		if (addr == 0) break;
		Serial.print(" Speed of : 0x"); Serial.print(addr, HEX); Serial.print(" is :"); Serial.println(i2C->getThisI2CFrequency(addr), DEC);
	}	

	Serial.println("\nSpeedtest Verify I2C\n");
	i2C->result.reset();
	while (i2C->scanNextS()) {
		i2C->speedTestS(&myVerifyTS);
	}
	
	Serial.println("\nWrite_Verify Speeds I2C:");
	for (int i = 0; i < 27; ++i) {
		auto addr = i2C->getAddress(i);
		if (addr == 0) break;
		Serial.print(" Speed of : 0x"); Serial.print(addr, HEX); Serial.print(" is :"); Serial.println(i2C->getThisI2CFrequency(addr), DEC);
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
	uint8_t hasFailed = rtc->writeEP(0x50, 0, sizeof(dataBuffa), dataBuffa);

	for (char & chr : dataBuffa) { chr = 0; }

	Serial.println("\nRead I2C_EEPROM :");
	if (!hasFailed) hasFailed = rtc->readEP(0x50, 0, sizeof(dataBuffa), dataBuffa);
	Serial.println(dataBuffa);
	Serial.println("\nFinished");
}

void loop() {

}