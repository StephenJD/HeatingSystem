#include "debgSwitch.h"
#include "DateTime_Run.h"
#include "I2C_Helper.h"
#include "Zone_Run.h"
#if defined ZPSIM
	#include "D_Factory.h"
	extern DTtype CURR_TIME;
#endif

extern I2C_Helper * rtc;

U1_byte rtc_timeout_reset(I2C_Helper & i2c, int addr){
	logToSD_notime("rtc_timeout_reset");
	rtc_reset();
	return false; // i2c.slowdown_and_reset();
}

void rtc_reset() {
	logToSD_notime("rtc_reset");
	pinMode(RTC_RESET,OUTPUT);
	digitalWrite(RTC_RESET, HIGH);
	delayMicroseconds(200000);
	digitalWrite(RTC_RESET, LOW);
	delayMicroseconds(200000);
	static bool doingReset = false;
	if (!doingReset) {
		doingReset = true;
		rtc->restart();
		currentTime(true);
		doingReset = false;
	}
}

void Curr_DateTime_Run::setTime(U1_byte chrs, U1_byte minsTens, U1_byte cday, U1_byte cmnth, U1_byte cyear) {
	mins = minsTens;
	hrs = chrs;
	day = cday;
	mnth = cmnth;
	year = cyear;
	minUnits(0);
	DPIndex = DPIndex | 16; // set autoDST;
	saveCurrDateTime(); 
}

void Curr_DateTime_Run::saveCurrDateTime() {
	uint8_t data[9];
	data[0] = 0; // seconds
	data[1] = (mins<<4) + minUnits();
	data[2] = toBCD(hrs);
	data[3] = 0; // day of week
	data[4] = toBCD(day);
	data[5] = toBCD(mnth);
	data[6] = toBCD(year);
	data[7] = 0; // disable SQW
	data[8] = DPIndex; // in RAM
	uint8_t errCode = rtc->write(RTC_ADDRESS,0,9,data);
	while (errCode != 0 && rtc->getI2CFrequency() > rtc->MIN_I2C_FREQ) {
		logToSD_notime("Error writing RTC : ", errCode);
		rtc->slowdown_and_reset(0);
		errCode = rtc->write(RTC_ADDRESS,0,9,data);
	} 	
	if (errCode != 0) {
		logToSD_notime("Unable to write RTC:",errCode);
	} else {
		logToSD_notime("Saved CurrDateTime");
	}
#if defined ZPSIM
	CURR_TIME = currentTime();
#endif
	Zone_Run::checkProgram(true);
}

void Curr_DateTime_Run::setFromCompiler() {
	DTtype compilerTime(__DATE__, __TIME__);
	mins = compilerTime.get10Mins();
	hrs = compilerTime.getHrs();
	day = compilerTime.getDay();
	mnth = compilerTime.getMnth();
	year = compilerTime.getYr();
	minUnits(compilerTime.minUnits());
	saveCurrDateTime();

	// Write date/time to user registers
	//uint8_t data[9];
	//rtc->write(RTC_ADDRESS,0,9,data); // Save Time
	//rtc->write(RTC_ADDRESS,0x08,9,data); // User registers
	logToSD_notime("RTC setFromCompiler day:", day);
}

U1_byte Curr_DateTime_Run::loadTime() {
	// called by logToSD so must not recursivly call logToSD
	uint8_t errCode = 0;
#if !defined (ZPSIM)
	if (rtc) {
//		int tryAgain = 10;
		uint8_t data[9];
//		do  {
			//data[1] = 0;
			//data[2] = 0;
			//data[4] = 0;
			//data[5] = 0;
			data[6] = 0;
			//errCode = rtc->read(RTC_ADDRESS,0x08,9,data); // Last Compiler Time
			
			errCode = rtc->read(RTC_ADDRESS,0,9,data);
			while ((errCode != 0 || data[6] == 255) && rtc->getI2CFrequency() > rtc->MIN_I2C_FREQ) {
				logToSD_notime("Error reading RTC : ", errCode);
				rtc->slowdown_and_reset(0);
				data[6] = 0;
				errCode = rtc->read(RTC_ADDRESS,0,9,data);
			} 			
			if (errCode != 0 ) {
				logToSD_notime("RTC Unreadable");
			} else if (data[6] == 0) {
				setFromCompiler();
				saveCurrDateTime();
			} else {
				mins = data[1] >> 4;
				hrs = fromBCD(data[2]);
				day = fromBCD(data[4]);
				mnth = fromBCD(data[5]);
				year = fromBCD(data[6]);
				DPIndex = data[8];
				minUnits(data[1] & 15);
				//logToSD_notime("Reading RTC year: ", year);
			}
//		} while (data[6] == 0 && --tryAgain > 0);
	} else {
		setFromCompiler();
		errCode = ERR_RTC_FAILED;
	}
#endif	
	return errCode;
}

U1_byte Curr_DateTime_Run::secondsSinceLastCheck(U4_byte & lastCheck){
	// move forward one second every 1000 milliseconds
	U4_byte elapsedTime = millisSince(lastCheck);
	U1_byte seconds = 0L;
	while (elapsedTime >= 1000L) {
		lastCheck += 1000L;
		elapsedTime -= 1000L;
		++seconds;
	}
	return seconds;
}

bool Curr_DateTime_Run::updateTime() { // returns true if program has changed. Called by arduino loop().
	bool returnVal = false;
	static U1_byte oldMins = mins+1; // force check first time through
	loadTime();
	
	if (mins != oldMins) { // every 10 mins
		oldMins = mins;
		adjustForDST();
		returnVal = Zone_Run::checkProgram(); // check for program changes
	}
	return returnVal;
}

void Curr_DateTime_Run::adjustForDST(){
	//Last Sunday in March and October. Go forward at 1 am, back at 2 am.
	if ((autoDST()) && (mnth == 3 || mnth == 10)) { // Mar/Oct
		if (day == 1 && dstHasBeenSet()){
			dstHasBeenSet(false); // clear
			saveCurrDateTime();
		} else if (day > 24) { // last week
			if (getDayNoOfDate() == 6) {
				if (!dstHasBeenSet() && mnth == 3 && hrs == 1) {
					hrs = 2;
					dstHasBeenSet(true); // set
					saveCurrDateTime();
				} else if (!dstHasBeenSet() && mnth == 10 && hrs == 2) {
					hrs = 1;
					dstHasBeenSet(true); // set
					saveCurrDateTime();
				}
			}
		} 
	}
}

void Curr_DateTime_Run::testAdd1Min() {
	if (minUnits() < 9) minUnits(minUnits()+1);
	else {
		minUnits(0);
		setDateTime(addDateTime(1));
	}
	updateTime();
}

bool Curr_DateTime_Run::dstHasBeenSet() const {
	return (DPIndex & 32) != 0;
}

void Curr_DateTime_Run::dstHasBeenSet(bool dstDone) {
	DPIndex = (DPIndex & ~32) | (dstDone * 32);
}

U1_byte Curr_DateTime_Run::autoDST() const {
	return (DPIndex & 16) != 0;
}

uint8_t Curr_DateTime_Run::toBCD(uint8_t value) const {
	uint8_t encoded = ((value / 10) << 4) + (value % 10);
	return encoded;
}

uint8_t Curr_DateTime_Run::fromBCD(uint8_t value) const {
	uint8_t decoded = value & 127;
	decoded = (decoded & 15) + 10 * ((decoded & (15 << 4)) >> 4);
	return decoded;
}
