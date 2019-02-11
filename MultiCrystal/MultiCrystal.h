// MultiCrystal.h: interface for the MultiCrystal class.
// Modified to include LCD via I2C
//////////////////////////////////////////////////////////////////////

#ifndef MultiCrystal_h
#define MultiCrystal_h

#include "Arduino.h"
//#include "inttypes.h"
#include "Print.h"
class I2C_Helper;

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

class MultiCrystal : public Print {
public:
	//MultiCrystal(); // default constructor
	// Constructors for parallel display
	MultiCrystal(uint8_t rs, uint8_t enable,
	uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3); // 6 parms - 4+2 pin

	MultiCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
	uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3); // 7 parms - 4+3 pin

	MultiCrystal(uint8_t rs, uint8_t enable,
	uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
	uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7); // 10 parms - 8+2 pin

	MultiCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
	uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
	uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7); // 11 parms - 8+3 pin
	
	MultiCrystal(uint8_t pinset[11]);

	// Read/Write Constructor for 8+3 pin via 1x16 I2C - 19 parms. Remaining 5 ports are for keypad
	MultiCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
	uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
	uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
	I2C_Helper * i2C, uint8_t address,
	uint8_t control_pos_is_port_B, uint8_t data_pos_is_port_B); // bits must be set for any data on port-B

	//void setI2C_Helper(I2C_Helper & i2C) { _i2C = &i2C; }

	uint8_t begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS); // returns >0 if failed
	using Print::print;
	size_t print(const char[]);
	size_t print(char);

#if defined (ZPSIM) // for simulator
	void print(char oneChar, byte size);

	void print (int);
	void print(unsigned long, int = DEC);
	void nextCol();
	char lcd_Arr[255];
	byte lcdPos;
	bool cursor_on,blink_on;
	bool dirty;
#endif

	uint8_t cursor_pos;
	uint8_t clear();
	uint8_t home();

	uint8_t noDisplay();
	uint8_t display();
	uint8_t noBlink();
	uint8_t blink();
	uint8_t noCursor();
	uint8_t cursor();
	uint8_t scrollDisplayLeft();
	uint8_t scrollDisplayRight();
	uint8_t leftToRight();
	uint8_t rightToLeft();
	uint8_t autoscroll();
	uint8_t noAutoscroll();

	uint8_t createChar(uint8_t, const uint8_t[]);
	uint8_t setCursor(uint8_t col, uint8_t row);
	virtual size_t write(uint8_t oneChar);
	uint8_t getError() {return errorCode;}
	
	uint8_t i2cAddress() {return _address;}
	uint8_t checkI2C_Failed();
	uint16_t readI2C_keypad(); // returns a single port reading per key-press

private:
	uint8_t _data[2]; // buffer for data sent to I2C ports
	uint8_t _key_mask[2]; // GPIO channels available for keypad
	const uint16_t & _key_mask_16; // = reinterpret_cast<uint16_t &>(_key_mask[0]);


	// Initialize Parallel
	void init(uint8_t fourbitmode, uint8_t rs, uint8_t rw, uint8_t enable,
	uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
	uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7);

	uint8_t send(uint8_t, uint8_t);
	uint8_t write4bits(uint8_t);
	uint8_t write8bits(uint8_t);
	uint8_t pulseEnable();
	uint8_t command(uint8_t); 

	void debug();
	
	uint8_t _numChars, _numCols;
	I2C_Helper * _i2C;
	uint8_t _address;
	uint8_t _control_pos_is_port_B;
	uint8_t _data_pos_is_port_B;
	
	uint8_t _rs_pin; // LOW: command. HIGH: character.
	uint8_t _rw_pin; // LOW: write to LCD. HIGH: read from LCD.
	uint8_t _enable_pin; // activated by a HIGH pulse.
	uint8_t _data_pins[8];
	
	uint8_t _displayfunction;
	uint8_t _displaycontrol;
	uint8_t _displaymode;

	uint8_t _initialized;
	//uint8_t _currline;
	
	// For I2C keypad

	//unsigned long _time_since_keychange;
	bool _keyCleared;

	// I2C stuff
	bool setControl(uint8_t port_pos, uint8_t value); // returns true if position is on port B
	void setDataBit(uint8_t position, uint8_t value);
	uint8_t errorCode;

};

#endif
