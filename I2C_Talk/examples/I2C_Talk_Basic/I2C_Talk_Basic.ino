#include <Arduino.h>
#include <Wire.h>
#include <I2C_Talk.h>
#include <I2C_Scan.h>

#define I2C_RESET 14


//////////////////////////////// Start execution here ///////////////////////////////
auto i2C = I2C_Talk();
auto scanner = I2C_Scan(i2C);

void setup() {
	Serial.begin(9600);
	Serial.println("Serial Begun");
	pinMode(I2C_RESET, OUTPUT);
	digitalWrite(I2C_RESET, HIGH); // reset pin

	Serial.println(" Create I2C_Talk");
}

void loop() {
	scanner.show_all();
}