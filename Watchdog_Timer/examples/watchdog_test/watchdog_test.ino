#include <PinObject.h>
#include <Watchdog_Timer.h>

using namespace HardwareInterfaces;

constexpr uint32_t SERIAL_RATE = 115200;
//constexpr int e_Status = LED_BUILTIN;
//auto led_Status = Pin_Wag(e_Status, HIGH);
//
//void setup() {
//  Serial.begin(SERIAL_RATE);
//  Serial.println("Start");
//  //set_watchdog_timeout_mS(8000);
//  set_watchdog_interrupt_mS(4000);
//
//  for (int i = 0; i < 5; ++i) { // signal a reset
//    led_Status.set();
//    delay(100);
//    led_Status.clear();
//    delay(100);
//  }
//
//  delay(500);
//  Serial.println("Setup complete");
//}
//
//void loop() {
//  // put your main code here, to run repeatedly:
//    led_Status.set();
//    delay(1000);
//    led_Status.clear();
//    delay(1000);
//}

/////////////////////////////////////////////////////////////////

void setup()
{
    //set_watchdog_interrupt_mS(2000);
    set_watchdog_timeout_mS(2000);
    Serial.begin(SERIAL_RATE);
}

void loop()
{

     Serial.println("Enter the main loop : Restart watchdog");

    while (true)
    {
        Serial.println("the software becomes trapped in a deadlock !");
        delay(1000);
        if (millis() < 8000) reset_watchdog(); 
        /* If the software becomes trapped in a deadlock,
           watchdog triggers a reset or an interrupt. if there is a reset, the software restarts with stored values
           in General Purpose Back up Registers*/
    }
}
void WDT_Handler(void)
{
    reset_watchdog(); 
    printf("help! in WDT\n"); // won't restart
    //while (true); // never restarts
}