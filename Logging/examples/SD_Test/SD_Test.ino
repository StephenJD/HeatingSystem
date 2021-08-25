#include <Arduino.h>
#include <SD.h>

#define SERIAL_RATE 115200

File dataFile;
constexpr int chipSelect = 53;

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(SERIAL_RATE); // NOTE! Serial.begin must be called before i2c_clock is constructed.
	Serial.flush();
  SD.begin(chipSelect);
	Serial.println("Serial.begin"); Serial.flush();

	//dataFile = SD.open("SD_Test.txt", FILE_WRITE); // appends to file

	if (SD.sd_exists(chipSelect,50)) {
		dataFile = SD.open("SD_Test.txt", FILE_WRITE); // appends to file
    dataFile.println("Reset SD OK");
    dataFile.flush();
		Serial.println("SD OK on start"); Serial.flush();
	} else 	Serial.println("SD Failed on start"); Serial.flush();


}

void loop()
{
	static bool lastState;
	static int okCount = 0, failedCount = 0;

	if (SD.sd_exists(chipSelect,50)) {
		dataFile = SD.open("SD_Test.txt", FILE_WRITE); // appends to file
		if (!lastState)
		{
			//Serial.print("SD OK "); Serial.println(++okCount); Serial.flush();
      //if  (!lastState) {
        dataFile.print("SD WasBad ");
        Serial.print("SD WasBad ");
      //}
		}      
    dataFile.print(++okCount); dataFile.println(" SD OK");
    dataFile.flush();

		lastState = true;
    if(okCount % 10 == 0) {
      Serial.print(okCount); Serial.println(" SD OK "); Serial.flush();
    }
	}
	else {
		dataFile = File{};
    //Serial.println("SD Failed "); 
		if (lastState)
		{
			/*if (lastState)*/ Serial.print("SD Was Good. ");
      Serial.print("SD Failed "); Serial.println(++failedCount); Serial.flush();
		}
		lastState = false;
	}
  delay(100);
}