/*
  TestBed for LCD Screen
 */
#include <Arduino.h>
#include <Wire.h>
#include <MultiCrystal.h>
#include <I2C_Talk.h>
#include <Conversions.h>
#include <Logging.h>
#include <SD.h>
#include <SPI.h>
#include <EEPROM.h>
#include <MemoryFree.h>

MultiCrystal * lcd(0);

Logger & logger() {
	static Logger _log{};
	return _log;
}

byte PHOTO_ANALOGUE =0;
byte BRIGHNESS_PWM =5;
byte CONTRAST_PWM =6;

byte dsBacklight = 255;
byte dsContrast = 50;
byte row = 0, col = 0;
char testChar = 'A';
//////////////////////////////// Start execution here ///////////////////////////////
void setup(){
	Serial.begin(9600);
	Serial.print("Serial Begin...");
   if (analogRead(0) < 10) {// old board; 
	PHOTO_ANALOGUE =1;
	BRIGHNESS_PWM =6;
	CONTRAST_PWM =7;
	lcd = new MultiCrystal(26,46,44, 42,40,38,36, 34,32,30,28); // old board
   } else {
	lcd = new MultiCrystal(46,44,42, 40,38,36,34, 32,30,28,26); // new board
   }
    // set up the LCD's number of rows and columns:
    lcd->begin(20,4);
     // set up the interrupts:
    analogWrite(BRIGHNESS_PWM, dsBacklight);  // Brightness analogRead values go from 0 to 1023, analogWrite values from 0 to 255
    analogWrite(CONTRAST_PWM, dsContrast);  // Contrast analogRead values go from 0 to 1023, analogWrite values from 0 to 255
 }

void loop(){
	lcd->clear();
    lcd->print("Two-Line-Word-Two-Line-Word");
    delay(2000);

	lcd->clear();
    lcd->print("Word L0");
    delay(2000);

	lcd->clear();
	lcd->setCursor(10,1);
    lcd->print("Mid L1");
    delay(2000);

	lcd->clear();
    lcd->print("Chars");
	char testChar = 'A';	
	for (int i = 5; i < 80; ++i) {
		delay(100);
		if (testChar == 'Z') testChar = 'A';
		lcd->print(testChar++);
	}

    delay(2000);
    lcd->clear();
    lcd->print("Set Cursor");
	testChar = 'A';
	for (int i = 10; i < 80; ++i) {
		delay(100); 
		lcd->setCursor(i % 20, i/20);
		if (testChar == 'Z') testChar = 'A';
		lcd->print(testChar++);
	}

    delay(2000);
    lcd->clear();
    lcd->print("Alternate");
	testChar = 'A';
	for (int i = 10; i < 80; i+=2) {
		delay(100); 
		lcd->setCursor(i % 20, i/20);
		if (testChar == 'Z') testChar = 'A';
		lcd->print(testChar++);
	}

    delay(2000);
    lcd->clear();
    lcd->print("NextLine");
	testChar = 'A';
	for (int L = 1; L < 4; ++L) {
		for (int c = 0; c < 20; c += 2) {
			delay(100);
			lcd->setCursor(c, L);
			if (testChar == 'Z') testChar = 'A';
			lcd->print(testChar++);
		}
	}
    delay(2000);
}


