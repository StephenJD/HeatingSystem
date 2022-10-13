/*
  TestBed for LCD Screen
 */
#include <Mega_Due.h>
#include <Arduino.h>
#include <Wire.h>
#include <MultiCrystal.h>
#include <I2C_Talk.h>
#include <Conversions.h>
#include <Logging.h>
#include <SD.h>
#include <SPI.h>
#include <EEPROM_RE.h>
#include <MemoryFree.h>
#include <LocalKeypad.h>

using namespace HardwareInterfaces;

MultiCrystal * lcd(0);
LocalKeypad keypad{ KEYPAD_INT_PIN, KEYPAD_ANALOGUE_PIN, KEYPAD_REF_PIN, { RESET_LEDN_PIN, LOW } };

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE);
		return _log;
	}
}
using namespace arduino_logger;

#if defined(__SAM3X8E__)
I2C_Talk rtc{ Wire1 }; // not initialised until this translation unit initialised.

EEPROMClassRE& eeprom() {
	static EEPROMClass_T<rtc> _eeprom_obj{ (rtc.ini(Wire1,100000),rtc.extendTimeouts(5000, 5, 1000), EEPROM_I2C_ADDR) }; // rtc will be referenced by the compiler, but rtc may not be constructed yet.
	return _eeprom_obj;
}
#endif

byte row = 0, col = 0;
char testChar = 'A';
//////////////////////////////// Start execution here ///////////////////////////////
void setup(){
	Serial.begin(SERIAL_RATE);
	Serial.print("Serial Begin...");
   if (megaFactor < 0.9) {// old board; 
		logger() << "Old Board" << L_flush;
		lcd = new MultiCrystal(26,46,44, 42,40,38,36, 34,32,30,28); // old board
   } else {
		logger() << "New Board" << L_flush;
		lcd = new MultiCrystal(46,44,42, 40,38,36,34, 32,30,28,26); // new board
   }
    // set up the LCD's number of rows and columns:
    lcd->begin(20,4);
	keypad.begin();
	analogWrite(BRIGHNESS_PWM, BRIGHTNESS);  // Brightness analogRead values go from 0 to 1023, analogWrite values from 0 to 255
	analogWrite(CONTRAST_PWM, contrast());  // Contrast analogRead values go from 0 to 1023, analogWrite values from 0 to 255
 }

void adjustBright_contrast() {
	static uint8_t _brightness = BRIGHTNESS;
	static uint8_t _contrast = contrast();
	auto nextLocalKey = keypad.popKey();
	while (nextLocalKey >= 0) { // only do something if key pressed
		logger() << "AnRef: " << analogRead(RESET_5vREF_PIN) << L_endl;
		switch (nextLocalKey) {
		case I_Keypad::KEY_UP:
			if (_brightness < 255/1.1) _brightness = (_brightness * 1.1) + 1;
			analogWrite(BRIGHNESS_PWM, _brightness);
			logger() << "_brightness : " << _brightness << L_endl;
			break;
		case I_Keypad::KEY_LEFT:
			if (_contrast > 2) _contrast/=1.1;
			analogWrite(CONTRAST_PWM, _contrast);
			logger() << "_contrast : " << _contrast << L_endl;
			break;
		case I_Keypad::KEY_RIGHT:
			if (_contrast < 255/1.1) _contrast = (_contrast * 1.1) + 1;
			analogWrite(CONTRAST_PWM, _contrast);
			logger() << "_contrast : " << _contrast << L_endl;
			break;
		case I_Keypad::KEY_DOWN:
			if (_brightness > 2) _brightness/=1.1;
			analogWrite(BRIGHNESS_PWM, _brightness);
			logger() << "_brightness : " << _brightness << L_endl;
			break;
		}
		lcd->setCursor(0, 0);
		lcd->print("bright ");
		lcd->print(_brightness);
		lcd->print(" ");
		lcd->setCursor(0, 1);
		lcd->print("contr ");
		lcd->print(_contrast);
		lcd->print(" ");
		lcd->setCursor(0, 2);
		lcd->print("Ref ");
		lcd->print(analogRead(RESET_5vREF_PIN));
		lcd->print(" ");
		nextLocalKey = keypad.popKey();
	}
}

void wait(int period_mS) {
	auto endTime = millis() + period_mS;
	while (endTime - millis() > 0) {
		adjustBright_contrast();
	}
}

void loop(){
	lcd->clear();
    lcd->print("Two-Line-Word-Two-Line-Word");
    wait(2000);

	lcd->clear();
    lcd->print("Word L0");
    wait(2000);

	lcd->clear();
	lcd->setCursor(10,1);
    lcd->print("Mid L1");
    wait(2000);

	lcd->clear();
    lcd->print("Chars");
	char testChar = 'A';	
	for (int i = 5; i < 80; ++i) {
		wait(100);
		if (testChar == 'Z') testChar = 'A';
		lcd->print(testChar++);
	}

    wait(2000);
    lcd->clear();
    lcd->print("Set Cursor");
	testChar = 'A';
	for (int i = 10; i < 80; ++i) {
		wait(100); 
		lcd->setCursor(i % 20, i/20);
		if (testChar == 'Z') testChar = 'A';
		lcd->print(testChar++);
	}

    wait(2000);
    lcd->clear();
    lcd->print("Alternate");
	testChar = 'A';
	for (int i = 10; i < 80; i+=2) {
		wait(100); 
		lcd->setCursor(i % 20, i/20);
		if (testChar == 'Z') testChar = 'A';
		lcd->print(testChar++);
	}

    wait(2000);
    lcd->clear();
    lcd->print("NextLine");
	testChar = 'A';
	for (int L = 1; L < 4; ++L) {
		for (int c = 0; c < 20; c += 2) {
			wait(100);
			lcd->setCursor(c, L);
			if (testChar == 'Z') testChar = 'A';
			lcd->print(testChar++);
		}
	}
    wait(2000);
}


