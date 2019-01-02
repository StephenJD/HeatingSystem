//#include <Due_Timer_Interrupts.h>

/*
  TestBed for Screen, relays, keyboard and I2C
*/
#include <Arduino.h>
#include <Wire.h>
#include <I2C_Helper.h>

#define I2C_RESET 14
#define RTC_RESET 4

I2C_Helper * i2C(0);
I2C_Helper * rtc(0);

uint8_t resetI2c(I2C_Helper & i2c, int) {
  Serial.println("Power-Down I2C...");
  digitalWrite(I2C_RESET, LOW);
  delayMicroseconds(5000);
  digitalWrite(I2C_RESET, HIGH);
  delayMicroseconds(5000);
  i2c.restart();
  return i2c.result.error;
}

uint8_t resetRTC(I2C_Helper & i2c, int) {
  Serial.println("Reset RTC...");
  digitalWrite(RTC_RESET, HIGH);
  delayMicroseconds(10000);
  digitalWrite(RTC_RESET, LOW);
  delayMicroseconds(10000);
  i2c.restart();
  return i2c.result.error;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
  Serial.begin(9600);
  Serial.println(" Serial Begun");
  pinMode(I2C_RESET, OUTPUT);
  digitalWrite(I2C_RESET, HIGH); // reset pin
  pinMode(RTC_RESET, OUTPUT);
  digitalWrite(RTC_RESET, LOW); // reset pin

  Serial.println(" Create i2C Helper");
  delay(100);
  i2C = new I2C_Helper(Wire, 10000);
  i2C->setTimeoutFn(resetI2c);
#if !defined NO_RTC
  Serial.println(" Create rtc Helper");
  rtc = new I2C_Helper(Wire1);
  rtc->setTimeoutFn(resetRTC);
  Serial.println(" Speedtest RTC");
  rtc->result.reset();

  while (rtc->scanNextS()) {
    rtc->speedTestS();
  }
#endif

  Serial.println(" Speedtest I2C");
  i2C->result.reset();
  while (i2C->scanNextS()) {
    i2C->speedTestS();
  }
}

void loop() {

}
