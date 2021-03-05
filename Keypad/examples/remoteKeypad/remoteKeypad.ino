#include <RemoteKeypad.h>
#include <RemoteDisplay.h>
#include <MultiCrystal.h>
#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <Logging.h>
#include <EEPROM.h>

using namespace HardwareInterfaces;
using namespace I2C_Recovery;

const uint8_t LOCAL_INT_PIN = 18;
const uint32_t SERIAL_RATE = 115200;
const uint8_t KEY_ANALOGUE = A1;
const uint8_t KEYPAD_REF_PIN = A3;
const uint8_t RESET_LEDP_PIN = 16;  // high supply
const uint8_t RESET_LEDN_PIN = 19;  // low for on.
const uint8_t REMOTE_ADDRESS = 0x26;
const uint8_t RESET_OUT_PIN = 14;  // active LOW.

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	return _log;
}

void ui_yield() {}

#if defined(__SAM3X8E__)
	#include <Clock_I2C.h>
	const uint8_t EEPROM_ADDRESS = 0x50;
	const uint8_t RTC_ADDRESS = 0x68;

	I2C_Talk rtc(Wire1);

	EEPROMClass& eeprom() {
		static EEPROMClass_T<rtc> _eeprom_obj{ (rtc.ini(Wire1,100000),rtc.extendTimeouts(5000, 5, 1000), EEPROM_ADDRESS) }; // rtc will be referenced by the compiler, but rtc may not be constructed yet.
		return _eeprom_obj;
	}

	Clock& clock_() {
		static Clock_I2C<rtc> _clock(RTC_ADDRESS);
		return _clock;
	}
#else
	#define EEPROM_CLOCK_ADDR 0
	#define EEPROM_CLOCK EEPROM_CLOCK_ADDR
	#include <Clock_EP.h>
	EEPROMClass & eeprom() {
	   return EEPROM;
	}
#endif

I2C_Talk i2C;
I2C_Recover recover(i2C);
RemoteDisplay rem_displ{recover, REMOTE_ADDRESS};
RemoteKeypad keypad{ rem_displ.displ() };

void turnOffRelays() {
	constexpr uint8_t REG_8PORT_IODIR = 0x00; // default all 1's = input
	constexpr uint8_t REG_8PORT_PullUp = 0x06;
	constexpr uint8_t REG_8PORT_OPORT = 0x09;
	constexpr uint8_t REG_8PORT_OLAT = 0x0A;

	uint8_t allZero[] = { 0 };
	uint8_t allOnes[] = { 0xFF };
	auto status = i2C.write(0x20, REG_8PORT_PullUp, 1, allZero); // clear all pull-up resistors
	logger() << "Relays PullUps" << i2C.getStatusMsg(status) << L_endl;
	status = i2C.write(0x20, REG_8PORT_OLAT, 1, allOnes); // set latches
	logger() << "Relays OLAT" << i2C.getStatusMsg(status) << L_endl;
	status = i2C.write(0x20, REG_8PORT_IODIR, 1, allZero); // set all as outputs
	logger() << "Relays IODIR" << i2C.getStatusMsg(status) << L_endl;
}

void setup()
{
	Serial.begin(SERIAL_RATE); // required for test-all.
	i2C.begin();
	i2C.setI2CFrequency(100000);
	pinMode(RESET_LEDP_PIN, OUTPUT);
	digitalWrite(RESET_LEDP_PIN, HIGH);
	pinMode(RESET_OUT_PIN, OUTPUT);
	digitalWrite(RESET_OUT_PIN, HIGH);
	logger() << L_allwaysFlush << L_time << F(" \n\n********** Logging Begun ***********") << L_endl;
	turnOffRelays();
	rem_displ.initialiseDevice();
    rem_displ.displ().print("Start ");
}

void loop()
{
	auto nextRemoteKey = keypad.getKey();
	while (nextRemoteKey >= 0) { // only do something if key pressed
		rem_displ.displ().clear();
		switch (nextRemoteKey) {
		case 0:
			logger() << "Info" << L_endl;
			break;
		case 1:
			logger() << "Up : " << L_endl;
			rem_displ.displ().print("Up");
			break;
		case 2:
			logger() << "Left : " << L_endl;
			rem_displ.displ().print("Left");
			break;
		case 3:
			logger() << "Right : " << L_endl;
			rem_displ.displ().print("Right");
			break;
		case 4:
			logger() << "Down : " << L_endl;
			rem_displ.displ().print("Down");
			break;
		case 5:
			logger() << "Back : " << L_endl;
			break;
		case 6:
			logger() << "Select : " << L_endl;
			break;
		default:
			logger() << "Unknown : " << nextRemoteKey << L_endl;
		}
		delay(100);
		nextRemoteKey = keypad.getKey();
		if (nextRemoteKey >= 0) logger() << "Second Key: ";
	}
	delay(2);
	//delay(5000); // allow queued keys
}