/******************************************************************************
   ISR_Switch.ino
   For Arduino AVR boards (UNO, Nano, Mega, etc. )
   Written by Khoi Hoang
  
   TCNTx - Timer/Counter Register. The actual timer value is stored here.
   OCRx - Output Compare Register
   ICRx - Input Capture Register (only for 16bit timer)
   TIMSKx - Timer/Counter Interrupt Mask Register. To enable/disable timer interrupts.
   TIFRx - Timer/Counter Interrupt Flag Register. Indicates a pending timer interrupt. 

   Now even you use all these new 16 ISR-based timers,with their maximum interval practically unlimited (limited only by
   unsigned long miliseconds), you just consume only one Hardware timer and avoid conflicting with other cores' tasks.
   The accuracy is nearly perfect compared to software timers. The most important feature is they're ISR-based timers
   Therefore, their executions are not blocked by bad-behaving functions / tasks.
   This important feature is absolutely necessary for mission-critical tasks.

   Based on SimpleTimer - A timer library for Arduino.
   Author: mromani@ottotecnica.com
   Copyright (c) 2010 OTTOTECNICA Italy

   Based on BlynkTimer.h
   Author: Volodymyr Shymanskyy
   
   Built by Khoi Hoang https://github.com/khoih-prog/TimerInterrupt_Generic
   Licensed under MIT license
*****************************************************************************************************************************/
/****************************************************************************************************************************
    ISR_Switch demontrates the use of ISR to avoid being blocked by other CPU-monopolizing task

   In this complex example: CPU is connecting to WiFi, Internet and finally Blynk service (https://docs.blynk.cc/)
   Many important tasks are fighting for limited CPU resource in this no-controlled single-tasking environment.
   In certain period, mission-critical tasks (you name it) could be deprived of CPU time and have no chance
   to be executed. This can lead to disastrous results at critical time.
   We hereby will use interrupt to detect whenever the SW is active, then switch ON/OFF a sample relay (lamp)
   We'll see this ISR-based operation will have highest priority, preempts all remaining tasks to assure its
   functionality.
*****************************************************************************************************************************/
/****************************************************************************************************************************
    This example is currently written for Arduino Mega 2560 with ESP-01 WiFi or Mega2560-WiFi-R3
    You can easily convert to UNO and ESP-01
    Mega: Digital pin 18 – 21,2 and 3 can be used to provide hardware interrupt from external devices.
    UNO/Nano: Digital pin 2 and 3 can be used to provide hardware interrupt from external devices.
    To upload program to MEGA2560+WiFi, only turn ON SW 3+4 (USB <-> MCU).
    To run MEGA+WiFi combined, turn ON SW 1+2 (MCU <-> ESP) and SW 3+4 (USB <-> MCU)
 *****************************************************************************************************************************/

// To be used with AVR Mega and Blynk v0.6.1 only
// Compiler error with Blynk v1.0.0

#warning To be used with AVR Mega and Blynk v0.6.1 only. Compiler error with Blynk v1.0.0

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG true

// These define's must be placed at the beginning before #include "TimerInterrupt_Generic.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0

#include "TimerInterrupt_Generic.h"

#if !(TIMER_INTERRUPT_USING_AVR)
  #error This is designed only for Arduino AVR board! Please check your Tools->Board setting.
#endif

#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>

#define BLYNK_HARDWARE_PORT     8080

#define USE_LOCAL_SERVER        true

// If local server
#if USE_LOCAL_SERVER
char blynk_server[]   = "yourname.duckdns.org";
//char blynk_server[]   = "192.168.2.110";
#else
char blynk_server[]   = "";
#endif

char auth[]     = "****";
char ssid[]     = "****";
char pass[]     = "****";

//Mega2560
// Hardware Serial on Mega, Leonardo, Micro...
#if ( ARDUINO_AVR_MEGA2560 || ARDUINO_AVR_MEGA || ARDUINO_AVR_ADK )
  #define EspSerial Serial3   //Serial1
  // Your HardwareSerial <-> ESP8266 baud rate:
  #define ESP8266_BAUD      115200
#elif ( defined(__AVR_ATmega328__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) )
  #include <SoftwareSerial.h>
  SoftwareSerial swSerial(2, 3);    // (RX, TX);
  #define EspSerial swSerial
  // Your HardwareSerial <-> ESP8266 baud rate:
  #define ESP8266_BAUD      9600
  #warning Using Software Serial for ESP with speed 9600 bauds
#else
  #define EspSerial Serial1
  // Your HardwareSerial <-> ESP8266 baud rate:
  #define ESP8266_BAUD      115200
#endif

ESP8266 wifi(&EspSerial);

#define DEBOUNCE_TIME               25
#define LONG_BUTTON_PRESS_TIME_MS   100
#define DEBUG_ISR                   0

#define VPIN                        V1
#define LAMPSTATE_PIN               V2

//Blynk Color in format #RRGGBB
#define BLYNK_GREEN     "#23C48E"
#define BLYNK_RED       "#D3435C"

WidgetLED  LampStatus(LAMPSTATE_PIN);

volatile unsigned long  lastDebounceTime  = 0;
volatile bool           buttonPressed     = false;
volatile bool           alreadyTriggered  = false;

volatile bool LampState    = false;
volatile bool SwitchReset  = true;

#define RELAY_PIN     A0        // Pin A0 of Mega2560 / UNO / Nano
#define BUTTON_PIN    2         // Pin 2 can be use for external interrupt source on Maga/UNO/Nano

BlynkTimer blynkTimer;

unsigned int myWiFiTimeout        =  3200L;  //  3.2s WiFi connection timeout   (WCT)
unsigned int buttonInterval       =  511L;  //   0.5s update button state

void Falling();
void Rising();

void lightOn();
void lightOff();
void ButtonCheck();
void ToggleRelay();

BLYNK_CONNECTED()
{
  LampStatus.on();
  Blynk.setProperty(LAMPSTATE_PIN, "color", LampState ? BLYNK_RED : BLYNK_GREEN );
  Blynk.syncVirtual(VPIN);
}

// Make this a autoreleased pushbutton
BLYNK_WRITE(VPIN)
{
  if (param.asInt())
    ToggleRelay();
}

void Rising()
{
  unsigned long currentTime  = millis();
  unsigned long TimeDiff;

  TimeDiff = currentTime - lastDebounceTime;
  if ( digitalRead(BUTTON_PIN) && (TimeDiff > DEBOUNCE_TIME) )
  {
    buttonPressed = false;
    ButtonCheck();
    lastDebounceTime = currentTime;
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), Falling, FALLING);
  }
}

void Falling()
{
  unsigned long currentTime  = millis();

  if ( !digitalRead(BUTTON_PIN) && (currentTime > lastDebounceTime + DEBOUNCE_TIME))
  {
    lastDebounceTime = currentTime;
    ButtonCheck();
    buttonPressed = true;
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), Rising, RISING);
  }
}

void heartBeatPrint()
{
  static int num = 1;

  Serial.print(F("B"));
  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(F(" "));
  }
}

void checkButton()
{
  heartBeatPrint();

  if (LampState)
    Blynk.setProperty(LAMPSTATE_PIN, "color", BLYNK_RED);
  else
    Blynk.setProperty(LAMPSTATE_PIN, "color", BLYNK_GREEN);
}

void ButtonCheck()
{
  boolean SwitchState = (digitalRead(BUTTON_PIN));

  if (!SwitchState && SwitchReset)
  {
    ToggleRelay();
    SwitchReset = false;
  }
  else if (SwitchState)
  {
    SwitchReset = true;
  }
}

void ToggleRelay()
{
  if (LampState)
    lightOff();
  else
    lightOn();
}

void lightOn()
{
  digitalWrite(RELAY_PIN, HIGH);
  LampState = true;
}

void lightOff()
{
  digitalWrite(RELAY_PIN, LOW);
  LampState = false;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  Serial.print(F("\nStarting ISR_Switch on "));
  Serial.println(BOARD_TYPE);
  Serial.println(TIMER_INTERRUPT_VERSION);
  Serial.println(TIMER_INTERRUPT_GENERIC_VERSION);
  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(RELAY_PIN, LOW);
  LampState = false;

  // Set ESP8266 baud rate
  EspSerial.begin(ESP8266_BAUD);
  delay(10);
 
  Serial.print(F("ESPSerial using ")); Serial.println(ESP8266_BAUD);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), Falling, FALLING);

  Blynk.begin(auth, wifi, ssid, pass, blynk_server, BLYNK_HARDWARE_PORT);

  if (Blynk.connected())
    Serial.println(F("Blynk connected"));
  else
    Serial.println(F("Blynk not connected yet"));

  // You need this timer for non-critical tasks. Avoid abusing ISR if not absolutely necessary.
  blynkTimer.setInterval(buttonInterval, checkButton);
}

void loop()
{
  Blynk.run();
  blynkTimer.run();
}