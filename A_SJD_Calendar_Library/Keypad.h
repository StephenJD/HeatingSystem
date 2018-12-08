#if !defined (_Keypad_H_)
#define _Keypad_H_

#include "debgSwitch.h"
#include "A__Constants.h"

class Display_Stream;
class Displays;

//////////////////////////////////////
///////////// Keypad ///////////////
//////////////////////////////////////

// Local Keypad notifies via int 5, 
// read via analogue 0,

//
// Remote keyboards notify via int 4,
// Check I2C addresses for Displays to find source
// Clear interrupt from display.
// Read key-pressed.

#define NUM_L_KEYS 7

S1_byte getFromKeyQue(volatile S1_byte * keyQue, volatile S1_byte & keyQuePos);
bool putInKeyQue(volatile S1_byte * keyQue, volatile S1_byte & keyQuePos, S1_byte key);

//enum {maxAnValsArr = 20};
//extern int anValIndex;
//extern S2_byte analogueVals[maxAnValsArr];
//extern int interruptCount;
//extern bool gotUp;


class Keypad {
public:
	virtual S1_byte getKey() = 0;
	virtual	S1_byte readKey() = 0;
	virtual bool isTimeToRefresh();
	#if defined (ZPSIM)
	static	S1_byte simKey;
	#endif
	volatile S1_byte keyQue[10];
	volatile S1_byte keyQuePos; // points to last entry in KeyQue
	bool wakeDisplay(bool wake);
	//virtual void setBackLight(bool wake) {}
	void setdisplayStream(Display_Stream *);
	Display_Stream * displayStream(){return _displayStream;}
protected:
	Keypad();
	S1_byte		timeSince;
	U4_byte		lastTick;
private:
	Display_Stream * _displayStream;
};

class LocalKeypad : public Keypad {
public:
	LocalKeypad(); //Display_Stream & display
	S1_byte getKey();
	bool isTimeToRefresh();

private:
	// members need to be static to allow attachment to interrupt handler
	static S2_byte adc_LocalKey_val[NUM_L_KEYS];
	S1_byte readKey();
	U1_byte get_key(U2_byte input);
	static S2_byte analogReadDelay(S1_byte an_pin);	
};

void localKeyboardInterrupt(); // must be a static or global

////////////// Remote Keypad ////////////////////////////

class RemoteKeypad : public Keypad {
public:
	RemoteKeypad(); // Display_Stream & display
	S1_byte readKey();
	S1_byte getKey();
//	bool isTimeToRefresh();

private:
	//U1_byte get_key(U2_byte input);
	S1_byte getKeyCode(U2_byte gpio);
};
#endif
