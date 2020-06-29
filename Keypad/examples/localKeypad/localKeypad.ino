#include <LocalKeypad.h>
#include <Logging.h>

using namespace HardwareInterfaces;
const uint8_t LOCAL_INT_PIN = 18;
const uint32_t SERIAL_RATE = 115200;
const uint8_t KEY_ANALOGUE = A1;
const uint8_t KEYPAD_REF_PIN = A3;
const uint8_t RESET_LEDP_PIN = 16;  // high supply
const uint8_t RESET_LEDN_PIN = 19;  // low for on.

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	return _log;
}

LocalKeypad keypad{ LOCAL_INT_PIN, KEY_ANALOGUE, KEYPAD_REF_PIN, { RESET_LEDN_PIN, LOW } };

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
	auto nextLocalKey = keypad.getKey();
	while (nextLocalKey >= 0) { // only do something if key pressed
		switch (nextLocalKey) {
		case 0:
			logger() << "Info" << L_endl;
			break;
		case 1:
			logger() << "Up : " << L_endl;
			break;
		case 2:
			logger() << "Left : " << L_endl;
			break;
		case 3:
			logger() << "Right : " << L_endl;
			break;
		case 4:
			logger() << "Down : " << L_endl;
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
		nextLocalKey = keypad.getKey();
		if (nextLocalKey >= 0) logger() << "Second Key: ";
	}
	delay(5000); // allow queued keys
}
