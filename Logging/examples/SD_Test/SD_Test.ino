#include <Arduino.h>
#include <SD.h>

#define SERIAL_RATE 115200

File dataFile;
constexpr int chipSelect = 53;

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(SERIAL_RATE); // NOTE! Serial.begin must be called before i2c_clock is constructed.
	SD.begin(chipSelect);
	Serial.println("Serial.begin");

	dataFile = SD.open("SD_Test.txt", FILE_WRITE); // appends to file

	if (SD.sd_exists(chipSelect)) {
		dataFile = SD.open("SD_Test.txt", FILE_WRITE); // appends to file
		Serial.println("SD OK on start");
	} else 	Serial.println("SD Failed on start");


}

void loop()
{
	static bool lastState;
	static uint8_t i;
	if (SD.sd_exists(chipSelect)) {
		dataFile = SD.open("SD_Test.txt", FILE_WRITE); // appends to file
		if (!lastState)
		{
			Serial.print("SD OK "); Serial.println(++i);
		}
		lastState = true;
	}
	else {
		dataFile = File{};
		if (lastState)
		{
			Serial.print("SD Failed "); Serial.println(++i);
		}
		lastState = false;
	}
}