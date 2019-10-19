#include "RelayM.h"
#include "Arduino.h"
#include <stdio.h>

void sendMsg(const char* msg);
void sendMsg(const char* msg, long a);

Relay::Relay(int address)
	: address(address)
{
	Serial.println("Relay created");
}

void Relay::set(bool on) {
	//Serial.print(address, DEC); Serial.print(" Relay::Set, on? "); Serial.println(on, DEC);
#ifdef TEST_MIX_VALVE_CONTROLLER
	extern int relayPin[4];
	relayPin[address] = on ? e_On : e_Off ;
#else
	sendMsg("Relay::set", address); 	
	digitalWrite(address, on ? e_On : e_Off);
#endif
}

bool Relay::get() {
#ifdef TEST_MIX_VALVE_CONTROLLER
	extern int relayPin[4];
	return relayPin[address] == e_On;
#else
	return digitalRead(address) == e_On;
#endif
}


