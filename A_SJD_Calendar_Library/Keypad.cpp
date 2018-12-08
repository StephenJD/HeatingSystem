#include "debgSwitch.h"
#include "Keypad.h"
#include "D_Factory.h"
#include "DateTime_Stream.h"

#include "B_Displays.h"
#include "I2C_Helper.h"
#include "Relay_Run.h"
#include "Display_Stream.h"
#include "B_Displays.h"

//////////////////////////////////////////////////////////////////////
// Class Keypad - statics
//////////////////////////////////////////////////////////////////////
#define LOCAL_INT_PIN 18

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  //Code in here will only be compiled if an Arduino Mega is used.
  #define KEYINT 5
#elif defined(__SAM3X8E__)
  #define KEYINT LOCAL_INT_PIN
#endif

#define KEY_ANALOGUE 1

//												   >info,Up, Lft,Rgt,Dwn,Bck,Sel
S2_byte LocalKeypad::adc_LocalKey_val[NUM_L_KEYS] = {874,798,687,612,551,501,440}; // for analogue keypad
static volatile bool keyQueueMutex = false;
//S2_byte analogueVals[maxAnValsArr];
//int anValIndex = 0;
//int interruptCount = 0;
//bool gotUp = false;
#if defined (ZPSIM)
	S1_byte Keypad::simKey;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Keypad::Keypad() : _displayStream(0) {
	//Display_Stream & displ
	keyQuePos = -1; // points to last entry in KeyQue
	for (U1_count i = 0; i < sizeof(keyQue); i++) {
		keyQue[i]= -1;
	}
	timeSince = 0;
	//logToSD("Keypad Constructor");
//	DateTime_Run::secondsSinceLastCheck(lastTick); // set lastTick for the keypad to now.
	//logToSD("Keypad Base Done");
}

void Keypad::setdisplayStream(Display_Stream * displayStream) {
	_displayStream = displayStream;
}


bool Keypad::isTimeToRefresh() {
	U1_byte elapsedTime = Curr_DateTime_Run::secondsSinceLastCheck(lastTick);
	if (elapsedTime && timeSince > 0) {
		timeSince -=  elapsedTime; // timeSince set to DISPLAY_WAKE_TIME whenever a key is pressed
	}
	return (elapsedTime > 0); // refresh every second
}

bool Keypad::wakeDisplay(bool wake) {
	if (wake) timeSince = DISPLAY_WAKE_TIME;
	return timeSince > 0;
 }

// ************************************************************************

LocalKeypad::LocalKeypad() { //Display_Stream & displ) : Keypad(displ) 
	digitalWrite(LOCAL_INT_PIN, HIGH); // turn ON pull-up 
	attachInterrupt(digitalPinToInterrupt(LOCAL_INT_PIN), localKeyboardInterrupt, FALLING); // attachInterrupt(KEYINT, localKeyboardInterrupt, CHANGE);
	logToSD("LocalKey Done");
}

#if defined (ZPSIM)
S1_byte LocalKeypad::readKey() {
	S1_byte retVal = simKey;
	simKey = -1;
	return retVal;
}
#else
S1_byte LocalKeypad::readKey() {
	// Cannot do Serial.print or logtoSD in here.
  	S2_byte adc_key_in = analogReadDelay(KEY_ANALOGUE);  // read the value from the sensor
	//S2_byte anRef = analogReadDelay(REF_ANALOGUE); // read Voltage Ref
	while (adc_key_in != analogReadDelay(KEY_ANALOGUE)) adc_key_in = analogReadDelay(KEY_ANALOGUE);
	// correct adc_key_in for non 5v ref
	//adc_key_in = S2_byte(adc_key_in * 1023L / anRef);
  	// Convert ADC value to key number
	S1_ind key;
	for (key = 0; key < NUM_L_KEYS; ++key) {
		if (adc_key_in > adc_LocalKey_val[key]) break;
	}

    if (key >= NUM_L_KEYS) key = -1;     // No valid key pressed
	return key;
}
#endif

S2_byte LocalKeypad::analogReadDelay(S1_byte an_pin) {
	delayMicroseconds(5000); // delay() does not work during an interrupt
	return analogRead(an_pin);   // read the value from the keypad
}

bool putInKeyQue(volatile S1_byte * keyQue, volatile S1_byte & keyQuePos, S1_byte myKey) {
	if (!keyQueueMutex && keyQuePos < 9 && myKey >=0) { // 
		++keyQuePos;
		keyQue[keyQuePos] = myKey;
		return true;
	} else return false;
}

S1_byte getFromKeyQue(volatile S1_byte * keyQue, volatile S1_byte & keyQuePos) {
	// "get" might be interrupted by "put" during an interrupt.
	S1_byte retKey = keyQue[0];
	if (keyQuePos >= 0)	{
		keyQueueMutex = true;
		if (keyQuePos > 0) { // keyQuePos is index of last valid key
			for (int i=0;i<keyQuePos;++i){
				keyQue[i] = keyQue[i+1];
			}
		} else {
			keyQue[0] = -1;
		}
		--keyQuePos;
		keyQueueMutex = false;
	}
	return retKey;
}

void localKeyboardInterrupt() { // static or global interrupt handler, no arguments
	static unsigned long lastTime =  millis();
	if (millis() < lastTime) return;
	lastTime = millis() + 2; // Required to prevent multiple interrupts from each key press.
	FactoryObjects * f = getFactory();
	if (f) {
		if (f->displayS(0).display()) {
			Keypad * keyPad = f->displayS(0).display()->keypad;
			if (keyPad) {
				if (keyPad->displayStream()) {
					S1_byte myKey = keyPad->readKey();
					S1_byte newKey = myKey;
					while (myKey == 1 && newKey == myKey){
						newKey = keyPad->readKey(); // see if a second key is pressed
						if (newKey >= 0) myKey = newKey; 
					}
					putInKeyQue(keyPad->keyQue, keyPad->keyQuePos, myKey);
				}
			}
		}
	}
}

S1_byte LocalKeypad::getKey() {
	S1_byte returnKey = getFromKeyQue(keyQue, keyQuePos);
	if (returnKey >= 0) {
#if !defined (ZPSIM)
		if (timeSince == 0) returnKey = -1;
#endif
		wakeDisplay(true);
		displayStream()->setBackLight(true);
	}
	return returnKey;
}

bool LocalKeypad::isTimeToRefresh() {
	bool doRefresh = Keypad::isTimeToRefresh(); // refresh every second
	if (timeSince <= 0) displayStream()->setBackLight(false);
	return doRefresh; 
}

//////////////// Remote Display ////////////////////////////////

RemoteKeypad::RemoteKeypad() {} //Display_Stream & displ) : Keypad(displ) {}

#if defined (ZPSIM)
S1_byte RemoteKeypad::readKey() {
	S1_byte myKey = simKey;
	putInKeyQue(keyQue,keyQuePos,myKey);
	simKey = -1;
	return myKey;
}
#else
S1_byte RemoteKeypad::readKey() {
	S1_byte myKey = getKeyCode(displayStream()->display()->lcd()->readI2C_keypad());
	if (myKey == -2) {
		logToSD("Remote Keypad: multiple keys detected");
	}
	if (putInKeyQue(keyQue,keyQuePos,myKey)) {
		logToSD("Remote Keypad Read Key. Add : ",myKey);
	}
	return myKey;
}

S1_byte RemoteKeypad::getKeyCode(U2_byte gpio) { // readI2C_keypad() returns 16 bit register
	S1_byte myKey;
	switch (gpio) {
	case 0x01: myKey = 2; break; //rc->clear(); rc->print("Left"); break;
	case 0x20: myKey = 4; break; // rc->clear(); rc->print("Down"); break;
	case 0x40: myKey = 1; break; // rc->clear(); rc->print("Up"); break;
	case 0x80: myKey = 3; break; // rc->clear(); rc->print("Right"); break;
	case 0:    myKey = -1; break; 
	default: myKey = -2; // multiple keys
	}
	return myKey;
}
#endif

S1_byte RemoteKeypad::getKey(){
	S1_byte gotKey = readKey();
	if (gotKey >= 0) {
		wakeDisplay(true);
		gotKey = getFromKeyQue(keyQue, keyQuePos);
	}
	return gotKey;
}