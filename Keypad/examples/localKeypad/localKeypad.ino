#include <Mega_Due.h>
#include <LocalKeypad.h>
#include <Logging.h>

using namespace HardwareInterfaces;

namespace arduino_logger {
  Logger & logger() {
    static Serial_Logger _log(SERIAL_RATE);
    return _log;
  }
}
using namespace arduino_logger;

LocalKeypad keypad{ KEYPAD_INT_PIN, KEYPAD_ANALOGUE_PIN, KEYPAD_REF_PIN, { RESET_LEDN_PIN, LOW } };

void setup()
{
	Serial.begin(SERIAL_RATE); // required for test-all.
	pinMode(RESET_LEDP_PIN, OUTPUT);

	digitalWrite(RESET_LEDP_PIN, HIGH);
	logger() << L_allwaysFlush << L_time << F(" \n\n********** Logging Begun ***********") << L_endl;
	keypad.begin();
}

void loop()
{
  auto nextLocalKey = keypad.popKey();
	//logger() << "AnRef: " << analogRead(KEYPAD_REF_PIN) << " AnRead(0): " << analogRead(A0) << " AnRead(1): " << analogRead(A1) << " AnRead(3): " << analogRead(A3) << L_endl;
	while (nextLocalKey >= 0) { // only do something if key pressed
		switch (nextLocalKey) {
		case I_Keypad::KEY_INFO:
			logger() << "Info" << L_endl;
			break;
		case I_Keypad::KEY_UP:
			logger() << "Up : " << L_endl;
			break;
		case I_Keypad::KEY_LEFT:
			logger() << "Left : " << L_endl;
			break;
		case I_Keypad::KEY_RIGHT:
			logger() << "Right : " << L_endl;
			break;
		case I_Keypad::KEY_DOWN:
			logger() << "Down : " << L_endl;
			break;
		case I_Keypad::KEY_BACK:
			logger() << "Back : " << L_endl;
			break;
		case I_Keypad::KEY_SELECT:
			logger() << "Select : " << L_endl;
			break;
		default:
			logger() << "Unknown : " << nextLocalKey << L_endl;
		}
		nextLocalKey = keypad.popKey();
		if (nextLocalKey >= 0) logger() << "Second Key: ";
	}
	//delay(5000); // allow queued keys
}
