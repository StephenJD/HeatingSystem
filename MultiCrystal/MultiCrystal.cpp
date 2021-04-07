#include "MultiCrystal.h"
#include <I2C_Talk_ErrorCodes.h>
#include "Arduino.h"
#include <I2C_Device.h>
#include <Conversions.h>
#include <Logging.h>
#include <MemoryFree.h>
using namespace GP_LIB;
using namespace I2C_Talk_ErrorCodes;

#if defined (ZPSIM)
using namespace std;
#include <stdio.h>
#include <string.h>
//void digitalWrite (uint8_t, uint8_t){;}
//void pinMode (uint8_t, uint8_t){;}
//void delayMicroseconds(unsigned int us){};
#endif

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
// DL = 1; 8-bit interface data
// N = 0; 1-line display
// F = 0; 5x8 dot character font
// 3. Display on/off control:
// D = 0; Display off
// C = 0; Cursor off
// B = 0; Blinking off
// 4. Entry mode set:
// I/D = 1; Increment by 1
// S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// MultiCrystal constructor is called).

// Keypad uses INTCAP to record a key-press. The register is read at regular intervals to see if a key was pressed.

constexpr uint8_t IOCON = 0x0A;	  // IO CONfiguration. Including INT polarity and whether INT is cleared by reading INTCAP or GPIO
constexpr uint8_t IODIR = 0x00;   // Sets IO DIRection - input or output.
constexpr uint8_t IPOL = 0x02;    // Sets GPIO Polarity - Set to invert input value in the GPIO register.
constexpr uint8_t DEFVAL = 0x06;  // Interrupt ocurrs if GPIO not DEFault VALue if INTCON bit is set. DEFVAL is the possibly inverted GPIO val, not the inputval that caused it.
constexpr uint8_t INTCON = 0x08;  // INTerrupt CONtrol. SET causes interrupt on NOT DEFVAL, CLEAR causes interrupt on GPIO changed.
constexpr uint8_t GPPU = 0x0C;    // GPio Pull Up
constexpr uint8_t GPINTEN = 0x04; // enables interrupt for GPIO port.
constexpr uint8_t INTCAP = 0x10;  // INTerrupt CAPture - Stores GPIO state at interrupt. Cleared when GPIO or INTCAP read (depending on INTCON).
constexpr uint8_t GPIOA = 0x12;   // OLATA = 0x14
constexpr uint8_t GPIOB = 0x13;   // OLATB = 0x15
constexpr uint8_t INTCAPtoClear_Mirror_INTlow = 0x01 | 0x64; // OR INT pins 'cos INTB is connected to GPIO.
constexpr uint8_t INTCAPtoClear_Mirror_INThigh = 0x03 | 0x64; // OR INT pins 'cos INTB is connected to GPIO.
constexpr uint8_t GPIOtoClear_Mirror_INTlow = 0x00 | 0x64;   // OR INT pins 'cos INTB is connected to GPIO.
constexpr uint8_t PULL_UP[2] = {0xFF,0xFF};
constexpr uint8_t CLEAR[2] = {0x0,0x0};
constexpr uint16_t PULL_UP_16 = 0xFFFF;
constexpr int IOPIN_CONNECTED_TO_INTA = 0x0200;

// Constructors for Parallel LCD
MultiCrystal::MultiCrystal(uint8_t rs,  uint8_t enable,
uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) // 6 parms - 4+2 pin
{
	init(1, rs, 255, enable, d0, d1, d2, d3, 0, 0, 0, 0);
}

MultiCrystal::MultiCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) // 7 parms - 4+3 pin
{
	init(1, rs, rw, enable, d0, d1, d2, d3, 0, 0, 0, 0);
}

MultiCrystal::MultiCrystal(uint8_t rs, uint8_t enable,
uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) // 10 parms - 8+2 pin
{
	init(0, rs, 255, enable, d0, d1, d2, d3, d4, d5, d6, d7);
}

MultiCrystal::MultiCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) // 11 parms - 8+3 pin
{
	init(0, rs, rw, enable, d0, d1, d2, d3, d4, d5, d6, d7);
	logger() << F("MultiCrystal ini[11]\n");
}

MultiCrystal::MultiCrystal(uint8_t pinset[11])
{
	init(0, pinset[0], pinset[1], pinset[2], pinset[3], pinset[4], pinset[5], pinset[6], pinset[7], pinset[8], pinset[9], pinset[10]);
	logger() << F("MultiCrystal ini[pinset]\n");
}


//*******************************************************************
// Any register initialization required must be done prior to calling this constructor //
//*******************************************************************

// Constructors for I2C LCD
MultiCrystal::MultiCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
	uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
	uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
	I_I2Cdevice * i2C_device, uint8_t address,
	uint8_t control_pos_is_port_B, uint8_t data_pos_is_port_B) // bits must be set for any data on port-B
	: 
	_i2C_device(i2C_device), _address(address),
	_control_pos_is_port_B(control_pos_is_port_B), 
	_data_pos_is_port_B(data_pos_is_port_B)
	{
		init(0, rs, rw, enable, d0, d1, d2, d3, d4, d5, d6, d7);
	}

// Initialise Display
void MultiCrystal::init(uint8_t fourbitmode, uint8_t rs, uint8_t rw, uint8_t enable,
uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
	_rs_pin = rs;
	_rw_pin = rw;
	_enable_pin = enable;
	
	_data_pins[0] = d0;
	_data_pins[1] = d1;
	_data_pins[2] = d2;
	_data_pins[3] = d3; 
	_data_pins[4] = d4;
	_data_pins[5] = d5;
	_data_pins[6] = d6;
	_data_pins[7] = d7;
	
	if (_i2C_device) { // create keypad mask.
		_data[0] = 0;
		_data[1] = 0;
		// following functions set bits in _data[]
		setControl(_rs_pin,1);
		setControl(_rw_pin,1);
		setControl(_enable_pin,1);
		for (int i=0;i<8;++i){
			setDataBit(i,1);
		}
		_key_mask[0] = ~_data[0];
		_key_mask[1] = ~_data[1];
		_key_mask_16 = I2C_Talk::fromBigEndian(_key_mask);
		_key_mask_16 &= ~IOPIN_CONNECTED_TO_INTA;
		//log("MultiCrystal::init for",_address," _key_mask_16", _key_mask_16);
	} else {  
		pinMode(_rs_pin, OUTPUT);
		// we can save 1 pin by not using RW. Indicate by passing -1 instead of pin#
		if (_rw_pin != -1) {pinMode(_rw_pin, OUTPUT);}
		pinMode(_enable_pin, OUTPUT);
	}

	if (fourbitmode) {
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	} else {
	_displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
	}
	_numCols = 0;
	_numChars = 0;
	cursor_pos = 0;
	#if defined (ZPSIM) // for simulator
	lcdPos = 0;
	cursor_on = false;
	blink_on = false;
	dirty = false;
	#endif
}

uint8_t MultiCrystal::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) { // I2C display
	if (lines > 1) _displayfunction |= LCD_2LINE;
	_numChars = lines * cols;
	_numCols = cols;
	//_currline = 0;
	//_keyCleared = true;
	//_lastKeyPress = 0;
	//_time_since_keychange = micros();
	cursor_pos = 0;
	
	#if defined (ZPSIM) // for simulator
	lcd_Arr[_numChars] = 0;
	cursor_on = false;
	blink_on = false;
	lcdPos =0;
	#endif
	
	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != 0) && (lines == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50mS
	delayMicroseconds(50000);
	// Now we pull both RS and R/W low to begin commands
	uint8_t hasFailed=0;
	if (_i2C_device == 0 ) {
		digitalWrite(_rs_pin, LOW);
		digitalWrite(_enable_pin, LOW);
		if (_rw_pin != -1) digitalWrite(_rw_pin, LOW);
	} else {
		// Lambdas
		// We use interrupt on GPIO change to record which key has been pressed, ready for retrieval when the keyboard is scanned.
		//auto setSingleBank = [this]() {_errorCode = 2; return _i2C_device->I_I2Cdevice::write(IOCON, 1, &GPIOtoClear_Mirror_INTlow); };  // IO CONfiguration: 0x01 = Single Bank, Clear INT on reading GPIO
		auto setIOCON = [this]() {_errorCode = 2; return _i2C_device->I_I2Cdevice::write(IOCON, 1, &INTCAPtoClear_Mirror_INTlow); };  // IO CONfiguration: 0x01 = Single Bank, Clear INT on reading GPIO
		auto setIO_Direction = [this]() {_errorCode = 3; return _i2C_device->I_I2Cdevice::write(IODIR, 2, _key_mask); };
		auto setIO_Polarity = [this]() {_errorCode = 4; return _i2C_device->I_I2Cdevice::write(IPOL, 2, _key_mask); };			// GPIO catures inverse of input for keyboard.
		auto setDefaut_IOstate = [this]() {_errorCode = 5; return  _i2C_device->I_I2Cdevice::write(DEFVAL, 2, CLEAR); };			// DEF val is the inverted input val - i.e. 0 for no key.
		auto setInterrupt_Control = [this]() {_errorCode = 6; return  _i2C_device->I_I2Cdevice::write(INTCON, 2, CLEAR); };	// INT on change - otherwise get multiple INT while key is down.
		auto setPullUps = [this]() {_errorCode = 7; return  _i2C_device->I_I2Cdevice::write(GPPU, 2, PULL_UP); };
		auto setInterruptEnable = [this]() {
			_errorCode = 8;
			bool status = _i2C_device->I_I2Cdevice::write(GPINTEN, 2, I2C_Talk::toBigEndian(_key_mask_16)());
			status = status || _i2C_device->read(GPINTEN, 2, _data); // recovery-Check GPINTEN is correct
			auto thisData = I2C_Talk::fromBigEndian(_data);
			return thisData == _key_mask_16 ? _OK : _I2C_ReadDataWrong;
		};	// INT enabled for keyboard.
		
		auto clearInterrupts = [this, setInterruptEnable, setIOCON]() {
			_errorCode = 9;
			// To clear INTCAP trigger int while only IOPIN_CONNECTED_TO_INTA is set.
			// a) Enable INT on IOPIN_CONNECTED_TO_INTA
			bool hasFailed = _i2C_device->I_I2Cdevice::write(GPINTEN, 2, _key_mask) != _OK;
			// b) Clear INTs.
			hasFailed = hasFailed || _i2C_device->I_I2Cdevice::read(INTCAP, 2, _data);
			// c) Toggle IOCON INT polarity to trigger INT while rest of GPIO is clear.
			hasFailed = hasFailed || _i2C_device->I_I2Cdevice::write(IOCON, 1, &INTCAPtoClear_Mirror_INThigh);
			hasFailed = hasFailed || setIOCON();
			// d) call setInterruptEnable() to disable INT on IOPIN_CONNECTED_TO_INTA
			hasFailed = hasFailed || setInterruptEnable();
			// f) clear INT
			hasFailed = hasFailed || _i2C_device->I_I2Cdevice::read(INTCAP, 2, _data);
			return hasFailed;
		};

		// Algorithm

		//Serial.print("Multi-Crystal Begin() Remote at 0x"); Serial.println(_address,HEX);
		_i2C_device->i2C().setI2CFrequency(_i2C_device->runSpeed()); // non-recovery read/writes
		_errorCode = 1;
		hasFailed = setIOCON() // 2
			|| setIO_Direction()	// 3
			|| setIO_Polarity()	// 4
			|| setDefaut_IOstate()	// 5
			|| setInterrupt_Control() // 6
			|| setPullUps()			// 7
			|| clearInterrupts();	// 9

		if (hasFailed) {
			//Serial.print("Multi-Crystal Failed Begin() at "); Serial.println(_errorCode);
			logger() << L_time << "Remote Multi-Crystal SetRegisters Failed device 0x" << L_hex << _i2C_device->getAddress() <<  L_endl;
			return hasFailed;
		} //else logger() << "Remote Multi-Crystal SetRegisters OK 0x" << L_hex << _i2C_device->getAddress() << L_endl;
		_data[0] = 0; _data[1] = 0;
	}

	if (!(_displayfunction & LCD_8BITMODE)) {
		//put the LCD into 4 bit or 8 bit mode
		// this is according to the hitachi HD44780 datasheet
		// figure 24, pg 46

		// we start in 8bit mode, try to set 4 bit mode
		_errorCode = 10;
		write4bits(0x03);
		delayMicroseconds(4500); // wait min 4.1ms

		// second try
		write4bits(0x03);
		delayMicroseconds(4500); // wait min 4.1ms

		// third go!
		hasFailed = write4bits(0x03);
		delayMicroseconds(150);

		// finally, set to 4-bit interface
		hasFailed = hasFailed || write4bits(0x02);
	} else {
		// this is according to the hitachi HD44780 datasheet
		// page 45 figure 23
		_errorCode = 11;
		// Send function set command sequence
		command(LCD_FUNCTIONSET | _displayfunction);
		delayMicroseconds(4500); // wait more than 4.1ms

		// second try
		command(LCD_FUNCTIONSET | _displayfunction);
		delayMicroseconds(150);

		// third go
		command(LCD_FUNCTIONSET | _displayfunction);
	}

	// finally, set # lines, font size, etc.
	_errorCode = 12;
	hasFailed = command(LCD_FUNCTIONSET | _displayfunction);

	// turn the display on with no cursor or blinking default
	_errorCode = 13;
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	hasFailed |= display();

	// clear it off
	_errorCode = 14;
	clear();

	// Initialize to default text direction (for romance languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	// set the entry mode
	_errorCode = 15;
	hasFailed |= command(LCD_ENTRYMODESET | _displaymode);
	
	if (hasFailed) return _unknownError;
	else {
		_errorCode = _OK;
		return _OK;
	}	
}

/********** Simulator commands, for the user! */
#if defined (ZPSIM)

void MultiCrystal::print(char oneChar, uint8_t size){ // ignores size
	lcd_Arr[lcdPos] = oneChar;
	nextCol();
}

void MultiCrystal::nextCol(){
	if (_numCols==0) {return;}
	lcdPos++;
	if (lcdPos % _numCols == (lcdPos/_numCols)-1){
		lcd_Arr[lcdPos++] = ' '; //'\n';
	}
	if (lcdPos >= _numChars + _numChars/_numCols)	lcdPos = 0;
}

void MultiCrystal::print (int number){
	const char* valStr = intToString(number);
	//CString valStr;
	//valStr.Format("%d",number); // format string as an integer
	for (uint8_t i=0;i<strlen(valStr);i++){
		lcd_Arr[lcdPos] = valStr[i];
		//write(word[i]);
		nextCol();
	}
	dirty=true;
}

void MultiCrystal::print (unsigned long number, int){
	const char* valStr = intToString(int16_t(number));
	//CString valStr;
	//valStr.Format("%d",number); // format string as an integer
	for (uint8_t i=0;i<strlen(valStr);i++){
		lcd_Arr[lcdPos] = valStr[i];
		//write(word[i]);
		nextCol();
	}
	dirty=true;
}

#endif

uint8_t MultiCrystal::setCursor(uint8_t col, uint8_t row) {
#if defined (ZPSIM)
	lcdPos = row * _numCols + col + row;
	if (lcdPos >= _numChars + _numChars /_numCols) lcdPos = _numChars + _numChars / _numCols - 1;
	cursor_pos = lcdPos;
#else
	cursor_pos = row * _numCols + col;
#endif

  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  auto noOfRows = _numChars / _numCols;
  if ( row >= noOfRows) {
    row = noOfRows -1;    // we count rows starting w/
  } 
  return command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turns the underline cursor on/off
uint8_t MultiCrystal::noCursor() {
#if defined (ZPSIM)
	cursor_on = false;	dirty=true;
#endif
    _displaycontrol &= ~LCD_CURSORON;
    return command(LCD_DISPLAYCONTROL | _displaycontrol);
}

uint8_t MultiCrystal::cursor() {
#if defined (ZPSIM)
	cursor_on = true;	dirty=true;
#endif
    _displaycontrol |= LCD_CURSORON;
    return command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
uint8_t MultiCrystal::noBlink() {
#if defined (ZPSIM)
	blink_on = false;	dirty=true;
#endif
    _displaycontrol &= ~LCD_BLINKON;
    return command(LCD_DISPLAYCONTROL | _displaycontrol);
}

uint8_t MultiCrystal::blink() {
#if defined (ZPSIM)
	blink_on = true;	dirty=true;
#endif
    _displaycontrol |= LCD_BLINKON;
    return command(LCD_DISPLAYCONTROL | _displaycontrol);
}

uint8_t MultiCrystal::clear() {
#if defined (ZPSIM)
	for (int i = 0; i < _numChars + _numChars / _numCols;  i++) {
		lcd_Arr[i]= ' ';
	}
	lcdPos = 0;
	dirty=true;
#endif 
	cursor_pos = 0;
	command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
    delayMicroseconds(2000);  // this command takes a long time!
	return _errorCode;
}

/********** high level commands, for the user! */
size_t MultiCrystal::print(char chr) {
#ifdef ZPSIM
		lcd_Arr[lcdPos] = chr;
		dirty = true;
		nextCol();
#endif
	auto charsSent = uint8_t(Print::print(chr));
	if (chr) ++cursor_pos;
	if (cursor_pos >= _numChars) cursor_pos = 0;
	if (cursor_pos % _numCols == 0) {
		auto lineNo = cursor_pos / _numCols;
		setCursor(0, lineNo);
	}
	return charsSent;
}

size_t MultiCrystal::print(const char buffer[] ) {
#ifdef ZPSIM
	for (size_t i = 0; i < strlen(buffer); i++) {
		lcd_Arr[lcdPos] = buffer[i];
		write(buffer[i]);
		nextCol();
	}
	dirty = true;
#endif
	//logger() << F("MultiCrystal::print ") << buffer << L_endl;
	auto noOfChars = 0; 
	auto endChar = buffer + strlen(buffer);
	for (auto nextChar = buffer; nextChar < endChar; ++nextChar) {
		if (print(*nextChar) == 0) {
			logger() << L_time << "MultiCrystal Print error." << L_endl;
			break;
		}
		++noOfChars;
	}
	return noOfChars;
}

uint8_t MultiCrystal::home()
{
	command(LCD_RETURNHOME); // set cursor position to zero
	delayMicroseconds(2000); // this command takes a long time!
	return _errorCode;
}

// Turn the display on/off (quickly)
uint8_t MultiCrystal::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	return command(LCD_DISPLAYCONTROL | _displaycontrol);
}
uint8_t MultiCrystal::display() {
	_displaycontrol |= LCD_DISPLAYON;
	return command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
uint8_t MultiCrystal::scrollDisplayLeft(void) {
	return command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
uint8_t MultiCrystal::scrollDisplayRight(void) {
	return command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
uint8_t MultiCrystal::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	return command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
uint8_t MultiCrystal::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	return command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
uint8_t MultiCrystal::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	return command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
uint8_t MultiCrystal::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	return command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
uint8_t MultiCrystal::createChar(uint8_t location, const uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
	return _errorCode;
}

/*********** mid level commands, for sending data/cmds */

inline uint8_t MultiCrystal::command(uint8_t value) { // 0 = success
	_errorCode = send(value, LOW);
	return _errorCode;
}

inline size_t MultiCrystal::write(uint8_t value) { // 0 = error, 1 = success
	//Serial.print("Write:");Serial.println(value,HEX);
	_errorCode = send(value, HIGH);
	if (_errorCode > 0) return 0;
	else return 1;
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
uint8_t MultiCrystal::send(uint8_t value, uint8_t mode) { // 0 = success
	uint8_t error = 0;

	if (_i2C_device == 0 ) {
		digitalWrite(_rs_pin, mode);
		//logger() << F(" MultiCrystal::send RS: ") << mode << L_endl;
		// if there is a RW pin indicated, set it low to Write
		if (_rw_pin != -1) {
			digitalWrite(_rw_pin, LOW);
			//logger() << F(" MultiCrystal::send RW to pin ") << _rw_pin << L_endl;
		}

	} else {
		setControl(_rs_pin, mode); // rs
		setControl(_rw_pin, LOW); // rw
	}
	if (_displayfunction & LCD_8BITMODE) {
		error = write8bits(value);
	} else {
		error = write4bits(value>>4);
		if (error == _OK) error= write4bits(value);
	}
	return error; 
}

uint8_t MultiCrystal::pulseEnable(void) { // this function sends the data to the display
	uint8_t error = 0;
	if (_i2C_device == 0 ) {
		digitalWrite(_enable_pin, LOW);
		delayMicroseconds(1);
		digitalWrite(_enable_pin, HIGH);
		delayMicroseconds(1); // enable pulse must be >450ns
		digitalWrite(_enable_pin, LOW);
		delayMicroseconds(100); // commands need > 37us to settle
	} else {
		bool isBport = setControl(_enable_pin, HIGH);
		_data[0] &= ~_key_mask[0]; // Key-masked bits must be zero.
		_data[1] &= ~_key_mask[1];
		//_i2C_device->i2C().setI2CFrequency(_i2C_device->runSpeed());
		if (isBport) { // enable after data written
			error = _i2C_device->/*I_I2Cdevice::*/write(GPIOA,2,_data); // 0 = success
		} else {
			error = _i2C_device->/*I_I2Cdevice::*/write(GPIOB,1,&_data[1]); // 0 = success
			if (error == _OK) error = _i2C_device->/*I_I2Cdevice::*/write(GPIOA,1,&_data[0]);
		}
		setControl(_enable_pin, LOW); // disable
		if (isBport) {
			if (error == _OK) error = _i2C_device->/*I_I2Cdevice::*/write(GPIOB,1,&_data[1]);
		} else {
			if (error == _OK) error = _i2C_device->/*I_I2Cdevice::*/write(GPIOA,1,&_data[0]);
		}
		if (error) logger() << L_time << "MultiCrystal::pulseEnable device 0x" << L_hex << _i2C_device->getAddress() << I2C_Talk::getStatusMsg(error) << " at freq: " << L_dec << _i2C_device->i2C().getI2CFrequency() << L_endl;
	}
	return error;
}

uint8_t MultiCrystal::write4bits(uint8_t value) { // 0 = success
	for (int i = 0; i < 4; i++) {
		if (_i2C_device == 0 ) {
			pinMode(_data_pins[i], OUTPUT);
			digitalWrite(_data_pins[i], (value >> i) & 0x01);
		} else {
			setDataBit(i, (value >> i) & 0x01);
		}
	}
	return pulseEnable();
}

uint8_t MultiCrystal::write8bits(uint8_t value) { // 0 = success
	for (int i = 0; i < 8; i++) {
		if (_i2C_device == 0 ) {
			pinMode(_data_pins[i], OUTPUT);
			digitalWrite(_data_pins[i], (value >> i) & 0x01);
		} else {
			setDataBit(i, (value >> i) & 0x01);
		}
	} 
	return pulseEnable();
}

/************ New functions **********/
bool MultiCrystal::setControl(uint8_t port_pos, uint8_t value) { 
// returns true if position is on port B
	port_pos = 1<<port_pos;
	bool isBport = ((_control_pos_is_port_B & port_pos) != 0);
	uint8_t & data = (isBport) ? _data[1] : _data[0];
	if (value) data = data | port_pos;
	else data = data & (~port_pos);
	return isBport;
}

void MultiCrystal::setDataBit(uint8_t data_channel, uint8_t value) {
	uint8_t & data = (_data_pos_is_port_B & (1<<data_channel)) ? _data[1] : _data[0];
	data_channel = 1<<_data_pins[data_channel];
	if (value) data = data | data_channel;
	else data = data & (~data_channel);
}

uint8_t MultiCrystal::checkI2C_Failed() {
	if (_i2C_device->reEnable() == _OK) {
		//logger() << "checkI2C_Failed? ..." << L_endl;
		uint8_t error = _i2C_device->read(GPINTEN, 2, _data); // recovery-Check GPINTEN is correct
		uint16_t thisData = I2C_Talk::fromBigEndian(_data);
		if (error == _OK) {
			if (thisData != _key_mask_16) {
				logger() << L_time << F("Remote Display checkI2C device 0x") << L_hex << _i2C_device->getAddress() << " Wrong GPINTEN: 0x" << thisData << " at " << L_dec << _i2C_device->runSpeed() << L_endl;
				error = _I2C_ReadDataWrong;
			} else {
				//logger() << L_time << F("Remote Display Read OK.") << L_endl;
				error = _i2C_device->write(IODIR, 2, _key_mask); // recovery
			}
		} else logger() << L_time << F("Remote Display checkI2C device 0x") << L_hex << _i2C_device->getAddress() << I2C_Talk::getStatusMsg(error) << L_endl;
		return error;
	} else return _disabledDevice;
}

uint16_t MultiCrystal::readI2C_keypad() {
	/*
	* INTCAP does not clear to 00 when INT is cleared! It always holds the last INT received, even after disabling interrupts!
	*  To detect an interrupt we have to read INTA by connecting it to a spare pin on GPIOA and disable INT on that pin.
	* 	Op(Diff)CAP	GPIO	INTA(inverted)	Op(Ch)	CAP	GPIO	INTA(inverted)
	* 	PWR		00	00		0 (Clear)		PWR		00	00		0 (Clear)
	*	Up		40	40		1 (Set)			Up		40	40		1 (Set)
	*	0		40	02		1 (Set)			0		40	02		1 (Set)
	*	Rd-40	40	00		0 (Clear)		Rd-40	40	02		0 (Clear)
	*	Up/0	40	02		1 (Set)			IO 2>0	40	00		0 (Clear)
	*	Rd-40	40	00		0 (Clear)		Up/0	40	02		1 (Set)		
	*	Up/0	40	02		1 (Set)			IO 2>0	40	00		0 (Clear)
	*	To detect a key, read GPIO-INT-Pin. If set read CAP to see which, and clear INT. 
	*	Either DIFF or change may be used.
	*/

	constexpr int CONSECUTIVE_COUNT = 2;
	constexpr int MAX_TRIES = 6;
	int16_t keyPressed = 0;
	// Read Int-pin on GPIO
// **********  Disabled in int RemoteKeypad::readKey()
	int16_t gpio = 0;
	if (_i2C_device->isEnabled()) {
		//logger() << "readI2C_keypad" << L_endl;
		auto gpioErr = _i2C_device->read_verify_2bytes(GPIOA, gpio, CONSECUTIVE_COUNT, MAX_TRIES, IOPIN_CONNECTED_TO_INTA); // non-recovery.
		//auto gpioErr = _i2C_device->I_I2Cdevice::read(GPIOA, 2, _data); // non-recovery.
		if (gpioErr) {
			//logger() << L_time << "readI2C_keypad Error Reading GPIO device 0x" << L_hex << _i2C_device->getAddress() << I2C_Talk::getStatusMsg(gpioErr) << " at " << L_dec << _i2C_device->runSpeed() << L_endl;
			checkI2C_Failed(); // Recovery
		} else {
			if (gpio) { 
				//logger() << L_time << "readI2C_keypad Got gpio" << L_endl;
				//_i2C_device->i2C().setI2CFrequency(_i2C_device->runSpeed());
				//_errorCode = _i2C_device->I_I2Cdevice::read(INTCAP, 2, _data); // non-recovery to reduce frequency of error msgs. Reading INTCAP clears INT, but INTCAP retains last INT state.
				_errorCode = _i2C_device->read_verify_2bytes(INTCAP, keyPressed, CONSECUTIVE_COUNT, MAX_TRIES, _key_mask_16); // non-recovery to reduce frequency of error msgs. Reading INTCAP clears INT, but INTCAP retains last INT state.
				if (_errorCode) { 
					//logger() << L_time << "readI2C_keypad Error Reading INTCAP device 0x" << L_hex << _i2C_device->getAddress() << I2C_Talk::getStatusMsg(_errorCode) << L_endl;
					keyPressed = 0;
				} else { // KeyPressed is masked with 0xE100 (RUD0 000L 0000 0000)
					if (keyPressed == _key_mask_16) return 0;
					if (keyPressed && checkI2C_Failed()) { // Recovery
						//logger() << L_time << "readI2C_keypad Check Failed device 0x" << L_hex << _i2C_device->getAddress() << " Key: 0x" << (uint16_t)keyPressed << L_endl;
						keyPressed = 0;
					} else if (keyPressed & 0x8100) {
						if (_i2C_device->runSpeed() > 10000) _i2C_device->set_runSpeed(long(_i2C_device->runSpeed() * 0.8));
						logger() << L_time << "readI2C_keypad CheckOK but nonexistant Key device 0x" << L_hex << _i2C_device->getAddress() << " Key: 0x" << (uint16_t)keyPressed << " Slowed to: " << L_dec << _i2C_device->runSpeed() << L_endl;
						keyPressed = 0;
					}
				}
			}
		}
	}
	return keyPressed;
}