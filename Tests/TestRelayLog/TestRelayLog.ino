#include <I2C_Talk_ErrorCodes.h>
#include <I2C_Talk.h>
#include <i2c_Device.h>
#include <I2C_Scan.h>
#include <I2C_RecoverRetest.h>

#include <EEPROM.h>
#include <Clock_I2C.h>
#include <Logging.h>
#include <Wire.h>
#include <TempSensor.h>
#include <Relay_Bitwise.h>

#include <SPI.h>
#include <SD.h>

#include <MultiCrystal.h>
#include <Date_Time.h>
#include <Conversions.h>
#include <MemoryFree.h>


using namespace I2C_Talk_ErrorCodes;
using namespace I2C_Recovery;
using namespace HardwareInterfaces;

#define LOCAL_INT_PIN 18
#define SERIAL_RATE 115200
#define DISPLAY_WAKE_TIME 15
#define I2C_RESET 14
#define RTC_RESET 4
#define I2C_DATA 20

void ui_yield() {}

const signed char ZERO_CROSS_PIN = -15; // -ve = active falling edge.
const signed char RESET_OUT_PIN = -14;  // -ve = active low.
const signed char RESET_LEDP_PIN = 16;  // high supply
const signed char RESET_LEDN_PIN = 19;  // low for on.
const signed char RTC_ADDRESS = 0x68;
const signed char EEPROM_CLOCK_ADDR = 0;
const signed char RELAY_PORT_ADDRESS = 0x20;
const signed char MIX_VALVE_ADDRESS = 0x10;
// registers
const unsigned char DS75LX_HYST_REG = 2;
const unsigned char REG_8PORT_IODIR = 0x00;
const unsigned char REG_8PORT_PullUp = 0x06;
const unsigned char REG_8PORT_OPORT = 0x09;
const unsigned char REG_8PORT_OLAT = 0x0A;
const unsigned char DS75LX_Temp = 0x00; // two bytes must be read. Temp is MS 9 bits, in 0.5 deg increments, with MSB indicating -ve temp.
const unsigned char DS75LX_Config = 0x01;

#if defined(__SAM3X8E__)
#define DIM_LCD 200

I2C_Talk rtc{ Wire1 }; // not initialised until this translation unit initialised.

I_I2C_Scan rtc_scan{ rtc };

EEPROMClass & eeprom() {
	static EEPROMClass_T<rtc> _eeprom_obj{ (rtc.ini(Wire1), 0x50) }; // rtc will be referenced by the compiler, but rtc may not be constructed yet.
	return _eeprom_obj;
}

Clock & clock_() {
	static Clock_I2C<rtc> _clock(RTC_ADDRESS);
	return _clock;
}

const float megaFactor = 1;
const char * LOG_FILE = "I2C_Due.txt";
#else
//Code in here will only be compiled if an Arduino Mega is used.
#define DIM_LCD 135

EEPROMClass & eeprom() {
	return EEPROM;
}

Clock & clock_() {
	static Clock_EEPROM _clock(EEPROM_CLOCK_ADDR);
	return _clock;
}
I2C_Talk rtc{ Wire };
const float megaFactor = 3.3 / 5;
const char * LOG_FILE = "I2C_Mega.txt";
#endif

#define REMKEYINT 19

Logger & logger() {
	//static Serial_Logger _log(SERIAL_RATE, clock_());
	static SD_Logger _log(LOG_FILE, SERIAL_RATE, clock_());
	return _log;
}

I2C_Talk i2C;
auto i2c_recover = I2C_Recover_Retest{ i2C, 0 };
I_I2C_SpeedTestAll i2c_test{ i2C, i2c_recover };

class RelaysPortSequence : public RelaysPort {
public:
	using RelaysPort::RelaysPort;
	error_codes testDevice() override;
	error_codes testRelays();
	error_codes nextSequence();
};

namespace HardwareInterfaces {
	Bitwise_RelayController & relayController() {
		static RelaysPortSequence _relaysPort(0x7F, i2c_recover, RELAY_PORT_ADDRESS, ZERO_CROSS_PIN, RESET_OUT_PIN);
		return _relaysPort;
	}
	RelaysPortSequence & rPort = static_cast<RelaysPortSequence &>(relayController());
}

void setup()
{
	logger() << "Logger started" << L_endl;
}

void loop()
{


}

// ************** Relay Port *****************

error_codes RelaysPortSequence::testDevice() {
	error_codes status = RelaysPort::initialiseDevice();
	//mainLCD->setCursor(7, 1);
	//mainLCD->print("Relay OK     ");
	//if (status != _OK) {
	//	mainLCD->setCursor(13, 1);
	//	mainLCD->print("Ini Bad");
	//}
	return status;
}

error_codes RelaysPortSequence::testRelays() {
	error_codes status = initialiseDevice();
	//mainLCD->setCursor(7, 1);
	//mainLCD->print("Relay        ");
	//mainLCD->setCursor(13, 1);
	//if (status != _OK) mainLCD->print("Ini Bad");
	//else if (testMode != speedTestExistsOnly) {
	//	const int noOfRelays = sizeof(relays) / sizeof(relays[0]);
	//	unsigned char numberFailed = 0;
	//	for (unsigned char relayNo = 0; relayNo < noOfRelays; ++relayNo) {
	//		auto onStatus = _OK;
	//		auto offStatus = _OK;
	//		mainLCD->setCursor(13, 1);
	//		mainLCD->print(relayNo, DEC);
	//		logger() << "* Relay *" << relayNo << L_endl;
	//		relays[relayNo].set(1);
	//		onStatus = updateRelays();
	//		//setRelay(relayNo, 1);
	//		//onStatus = write_verify(REG_8PORT_OLAT, 1, &relayRegister);
	//		if (onStatus == _OK) logger() << " ON OK\n"; else logger() << " ON Failed\n";
	//		if (yieldFor(300)) break;
	//		relays[relayNo].set(0);
	//		//setRelay(relayNo, 0);
	//		offStatus = updateRelays();
	//		//offStatus = write_verify(REG_8PORT_OLAT, 1, &relayRegister);
	//		if (offStatus == _OK) logger() << " OFF OK\n"; else logger() << " OFF Failed\n";
	//		offStatus |= onStatus;
	//		numberFailed = numberFailed + (offStatus != _OK);
	//		if (offStatus != _OK) {
	//			mainLCD->print(" Bad  ");
	//			logger() << "Test Failed: " << relayNo << L_endl;
	//		}
	//		else {
	//			mainLCD->print(" OK   ");
	//			logger() << "Test OK\n";
	//		}
	//		if (yieldFor(300)) break;
	//		status |= offStatus;
	//	}
	//}
	return status;
}

error_codes RelaysPortSequence::nextSequence() {
	static int sequenceIndex = 0;
	//_relayRegister = relaySequence[sequenceIndex];
	//++sequenceIndex;
	//if (sequenceIndex >= sizeof(relaySequence)) sequenceIndex = 0;
	auto status = updateRelays();
	//mainLCD->setCursor(7, 1);
	//mainLCD->print("Relay ");
	//mainLCD->setCursor(13, 1);
	//mainLCD->print(~_relayRegister, BIN);
	//if (status != _OK) {
	//	logger() << "RelaysPort::nextSequence failed first write_verify" << L_endl;
	//	initialiseDevice();
	//	status = updateRelays();
	//	if (status != _OK) {
	//		logger() << "RelaysPort::nextSequence failed second write_verify" << L_endl;
	//		mainLCD->setCursor(13, 1);
	//		mainLCD->print("Bad   ");
	//	}
	//}
	return status;
}