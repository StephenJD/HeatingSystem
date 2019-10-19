#include <Arduino.h>
#include <I2C_Talk_ErrorCodes.h>
#include <I2C_Talk.h>
#include <i2c_Device.h>
#include <I2C_Scan.h>
#include <I2C_RecoverRetest.h>

#include <Clock.h>
#include <Logging.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SD.h>
#include <SPI.h>
#include <Conversions.h>
#include <Date_Time.h>

#define I2C_RESET 14
#define RTC_RESET 4

int st_index;

using namespace I2C_Recovery;

constexpr uint8_t TEMP_HYST_REG = 2;
constexpr uint8_t TEMP_LIMIT_REG = 3;

I2C_Talk rtc{ Wire1 };

EEPROMClass & eeprom() {
	static EEPROMClass_T<rtc> _eeprom_obj{ 0x50 };
	return _eeprom_obj;
}

EEPROMClass & EEPROM = eeprom();

Clock & clock_() {
	static Clock _clock;
	return _clock;
}

Logger & logger() {
	static Serial_Logger _log(9600, clock_());
	return _log;
}

I2C_Talk i2C;

auto i2c_recover = I2C_Recover_Retest{};
auto rtc_recover = I2C_Recover_Retest{};
I_I2C_Scan rtc_scan{rtc};
I_I2C_SpeedTestAll i2c_test{i2C, i2c_recover};


uint8_t resetI2c(I2C_Talk & i2c, int) {
	Serial.println("Power-Down I2C...");
	digitalWrite(I2C_RESET, LOW);
	delayMicroseconds(5000);
	digitalWrite(I2C_RESET, HIGH);
	delayMicroseconds(5000);
	i2c.restart();
	return 0;
}

uint8_t resetRTC(I2C_Talk & i2c, int addr) {
	//Serial.println("Power-Down RTC...");
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, HIGH);
	delayMicroseconds(200000);
	digitalWrite(RTC_RESET, LOW);
	i2c.restart();
	delayMicroseconds(200000);
	return true;
}

class I2C_TempSensor : public I_I2Cdevice_Recovery {
public:
	using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
	uint8_t testDevice() override {
		Serial.print("Test I2C_TempSensor at 0x"); Serial.println(getAddress(), HEX);
		uint8_t temp[2] = { 75,0 };
		return write_verify(2, 2, temp);
	}
};

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

I2C_TempSensor tempSens[27] = { {i2c_recover,1} };

char poem[] = {
"Twas brillig, and the slithy toves \nDid gyre and gimble in the wabe;\nAll mimsy were the borogoves,\
\nAnd the mome raths outgrabe. \n\nBeware the Jabberwock, my son! \nThe jaws that bite, the claws that catch!\
\nBeware the Jubjub bird, and shun \nThe frumious Bandersnatch!\
\n\nHe took his vorpal sword in hand: \nLong time the manxome foe he sought -\nSo rested he by the Tumtum tree,\
\nand stood awhile in thought.\n\nAnd as in uffish thought he stood, \nThe Jabberwock, with eyes of flame,\
\nCame whiffling through the tulgey wood,\nAnd burbled as it came!\n" };

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600);
	Serial.println("Serial Begun");
	pinMode(I2C_RESET, OUTPUT);
	digitalWrite(I2C_RESET, HIGH); // reset pin
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	i2C.restart();
	rtc.restart();

	Serial.print("Wire addr   "); Serial.println((long)&Wire);
	Serial.print("Wire 1 addr "); Serial.println((long)&Wire1);

	rtc_scan.show_all();

	i2c_recover.setTimeoutFn(resetI2c);
	i2c_test.showAll_fastest();

	Serial.println("\nSpeedtest Each addr\n");
	int tsIndex = 0;
	i2c_test.foundDeviceAddr = 0x28;
	while (i2c_test.next()) {
		Serial.print("\nSpeed Test for 0x"); Serial.print(i2c_test.foundDeviceAddr, HEX);
		tempSens[tsIndex].setAddress(i2c_test.foundDeviceAddr);
		tempSens[tsIndex].setRecovery(i2c_recover);

		tempSens[tsIndex].set_runSpeed(i2c_test.show_fastest());
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
		i2c_test.show_fastest(ts);
	}

	Serial.println("\nPost-Test Operational Speeds I2C:");
	for (auto & ts : tempSens) {
		auto addr = ts.getAddress();
		if (addr == 0) break;
		Serial.print(" Speed of : 0x"); Serial.print(addr, HEX); Serial.print(" is :"); Serial.println(ts.runSpeed(), DEC);
	}

	Serial.println("Write EEPROM :");
	auto hasFailed = rtc.writeEP(0x50, 30, sizeof(poem), poem);
	for (char & chr : poem) { chr = 0; }
	Serial.println("\nRead EEPROM :");
	if (!hasFailed) hasFailed = rtc.readEP(0x50, 0, sizeof(poem), poem);
	Serial.println(poem);
	Serial.println("\nFinished");
}

void loop() {
	for (char & chr : poem) { chr = 0; }

	Serial.println("\nRead EEPROM :");
	//status = EEPROM.readEP(30, sizeof(poem), poem);
	auto status = rtc.readEP(0x50, 30, sizeof(poem), poem);
	if (status) {
		Serial.print("readEP failed with"); Serial.println(rtc.getStatusMsg(status));
		rtc.restart();
	}

	Serial.println(poem);
	Serial.println();
}