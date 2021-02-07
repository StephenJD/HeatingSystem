#include <PinObject.h>
#include <Watchdog_Timer.h>

using namespace HardwareInterfaces;

constexpr uint32_t SERIAL_RATE = 115200;
constexpr int e_Status = 13;
auto led_Status = Pin_Wag(e_Status, HIGH);

void setup() {
  Serial.begin(SERIAL_RATE);
  Serial.println("Start");
  set_watchdog_timeout_mS(8000);

  for (int i = 0; i < 5; ++i) { // signal a reset
    led_Status.set();
    delay(100);
    led_Status.clear();
    delay(100);
  }

  delay(500);
  Serial.println("Setup complete");
}

void loop() {
  // put your main code here, to run repeatedly:
    led_Status.set();
    delay(1000);
    led_Status.clear();
    delay(1000);
}
