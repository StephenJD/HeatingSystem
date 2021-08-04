#include <DueTimer.h>

int myLed = 13;

bool ledOn = false;
auto dueTimer = Timer.getAvailable();

void myHandler(){
  dueTimer.stop();
	ledOn = !ledOn;
  Serial.println("Int");
	digitalWrite(myLed, ledOn); // Led on, off, on, off...
  dueTimer.start();
}

void setup(){
  Serial.begin(115200);
	pinMode(myLed, OUTPUT);

	dueTimer.attachInterrupt(myHandler);
  dueTimer.setPeriod(500000);
	dueTimer.start(); // Calls every 50ms
}

void loop(){

	while(1){
		// I'm stuck in here! help me...
	}
	
}