#pragma once

#include "D_Operations.h"
#include "D_EpData.h"
#include "DateTime_A.h"
//#include "DateTime_Stream.h"
class TwoWire;
U1_byte rtc_timeout_reset(I2C_Helper &,int);
void rtc_reset();

class DateTime_Run : public DTtype, public RunT<EpDataT<DTME_DEF> >
{
public:
	DateTime_Run() {};
private:
};

class Curr_DateTime_Run : public DTtype
{ // Runs the Real-Time Clock module
public:
	Curr_DateTime_Run() {};
	void setTime(U1_byte hrs, U1_byte minsTens, U1_byte day, U1_byte mnth, U1_byte year);
	bool updateTime(); // makes the clock tick!
	static U1_byte secondsSinceLastCheck(U4_byte &lastCheck);
	static U4_byte millisSince(U4_byte startTime) {return millis() - startTime;} // Since unsigned ints are used, rollover just works.
	void saveCurrDateTime();
	U1_byte loadTime();
	void testAdd1Min();
	
protected:
	U1_byte autoDST() const;

private:
	bool dstHasBeenSet() const;

	void dstHasBeenSet(bool);

	void adjustForDST(); // adjusts hour if date is clock-change day
	void setFromCompiler();

	U1_byte toBCD(U1_byte) const;
	U1_byte fromBCD(U1_byte) const;
};

//class TwoWire;
//class RTC_EEPROM {
//public:
	//RTC_EEPROM(TwoWire & wire_port, U1_byte rtcAddr, U1_byte eepromAddr);
	//I2C_Helper * _rtc_eeprom;
//};
