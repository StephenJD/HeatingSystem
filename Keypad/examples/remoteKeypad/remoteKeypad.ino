#define EEPROM_CLOCK_ADDR 0
#define NO_RTC
//#define EEPROM_CLOCK EEPROM_CLOCK_ADDR
#include <Clock_EP.h>

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
const uint8_t US_REMOTE_ADDRESS = 0x24;

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	return _log;
}

EEPROMClass & eeprom() {
   return EEPROM;
}

Clock& clock_() {
  static Clock_EEPROM _clock(EEPROM_CLOCK_ADDR);
  return _clock;
}

I2C_Talk i2C;
I2C_Recover recover(i2C);
RemoteDisplay rem_displ{recover, US_REMOTE_ADDRESS};
RemoteKeypad keypad{ rem_displ.displ() };

void setup()
{
	Serial.begin(SERIAL_RATE); // required for test-all.
	pinMode(RESET_LEDP_PIN, OUTPUT);
	digitalWrite(RESET_LEDP_PIN, HIGH);
	logger() << L_allwaysFlush << L_time << F(" \n\n********** Logging Begun ***********") << L_endl;
	rem_displ.initialiseDevice();
  rem_displ.displ().print("Start ");
}

void loop()
{
	auto nextLocalKey = keypad.getKey();
	while (nextLocalKey >= 0) { // only do something if key pressed
    rem_displ.displ().clear();
		switch (nextLocalKey) {
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
			logger() << "Unknown : " << nextLocalKey << L_endl;
		}
    delay(500);
		nextLocalKey = keypad.getKey();
		if (nextLocalKey >= 0) logger() << "Second Key: ";
	}
	delay(5000); // allow queued keys
}