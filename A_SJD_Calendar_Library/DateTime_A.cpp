#include "DateTime_A.h"
#include "D_Factory.h"
#include "A_Stream_Utilities.h"
#include "EEPROM_Utilities.h"

using namespace EEPROM_Utilities;
using namespace Utils;

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

DTtype currentTime(bool set) { // Called by rtc_reset. Probably not required - does updateTime() every second in setup_loop
	if (set) currTime().loadTime();
	return currTime().getDateTime();
}

// Global binary operators //
bool operator== (const DTtype& lhs, const DTtype& rhs) {
	return ((lhs & DTtype::dateTimeMask).S4() == (rhs & DTtype::dateTimeMask).S4());
}

bool operator< (const DTtype& lhs, const DTtype& rhs) {
	return ((lhs & DTtype::dateTimeMask).S4() < (rhs & DTtype::dateTimeMask).S4());
}

DTtype operator& (const DTtype& lhs, const DTtype& rhs) {
	return DTtype(lhs.S4() & rhs.S4());
}

DTtype operator| (const DTtype& lhs, const DTtype& rhs){
	return DTtype(lhs.S4() | rhs.S4());
}

// Constructors //

DTtype::DTtype(): mins(0), hrs(0),day(0),mnth(0),year(0),DPIndex(0) {}
DTtype::DTtype(U1_byte z): mins(z & 7), hrs(z/8),day(0),mnth(0),year(0),DPIndex(0) { if (z == 255) {day = U1_byte(-1); mnth = U1_byte(-1); year =  U1_byte(-1);}};
DTtype::DTtype(U1_byte y, U1_byte m, U1_byte d, U1_byte h, U1_byte n): mins(n), hrs(h),day(d),mnth(m),year(y),DPIndex(0) {};

DTtype::DTtype(const char* date, const char* time) {
	// sample input: date = "Dec 26 2009", time = "12:34:56"
    year = conv2d(date + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
    switch (date[0]) {
        case 'J': mnth = date[1] == 'a' ? 1 : mnth = date[2] == 'n' ? 6 : 7; break;
        case 'F': mnth = 2; break;
        case 'A': mnth = date[2] == 'r' ? 4 : 8; break;
        case 'M': mnth = date[2] == 'r' ? 3 : 5; break;
        case 'S': mnth = 9; break;
        case 'O': mnth = 10; break;
        case 'N': mnth = 11; break;
        case 'D': mnth = 12; break;
    }
    day = conv2d(date + 4);
    hrs = conv2d(time);
    mins = conv2d(time + 3) / 10;
	minUnits(conv2d(time + 3) % 10);
}
 
DTtype::DTtype(U4_byte longDT){ // DTtype constructor taking a long for loading from EEPROM.
	const void * vPtr = &longDT; // void pointer at longDT
	*this = *(reinterpret_cast<const DTtype*>(vPtr)); // cast void pointer as pointer to DTtype and assign. 
}

DTtype::DTtype(S4_byte longDT){ // DTtype constructor taking a long for loading from EEPROM.
	const void * vPtr = &longDT; // void pointer at longDT
	*this = *(reinterpret_cast<const DTtype*>(vPtr)); // cast void pointer as pointer to DTtype and assign. 
}

// operators //

DTtype DTtype::operator~ () const {
	return DTtype(~this->S4());
}

U4_byte DTtype::U4() const {
    const void * longDT = this; // void pointer at this
    return *(reinterpret_cast<const U4_byte *>(longDT)); // cast void pointer as pointer to long. 
}

S4_byte DTtype::S4() const {
    const void * longDT = this; // void pointer at this
    return *(reinterpret_cast<const S4_byte *>(longDT)); // cast void pointer as pointer to long. 
}

DTtype::operator int() const {return this->mins + this->hrs * 8 + this->day * 192;}

U1_byte DTtype::U1() const {return this->mins + this->hrs * 8 + this->day * 192;}

const DTtype DTtype::dateTimeMask(127,15,31,31,7);
const DTtype DTtype::dateMask(127,15,31,0,0);
const DTtype DTtype::timeMask(0,0,0,31,7);
const DTtype DTtype::JUDGEMEMT_DAY(127,15,31,0,0);
const DTtype DTtype::MAX_PERIOD(0,0,1,7,5);
const DTtype DTtype::MIDNIGHT(0,0,0,24,0);

DTtype DTtype::getDate() const {
	return *this & dateMask;
}

U1_byte DTtype::displayHrs(U1_byte hrs) {
	if (hrs > 12) hrs -= 12;
	if (hrs == 0) hrs = 12;
	return hrs;
}

U1_byte DTtype::getDayNoOfDate(U1_byte day, U1_byte month, U2_byte yy){ // static
	// returns day number. 0 = Monday
  	 yy = yy + 2000;
     if (month < 3) {
           month += 12;
           --yy;
     }
  	  
  	return (--day + (2 * month) + (6 * (month + 1) / 10) +
       		yy + (yy / 4) - (yy / 100) +
       		(yy / 400) + 1 ) %7;
}

U1_byte DTtype::getDayNoOfDate() const { // returns 0-6 with 0 == Monday.
	return getDayNoOfDate(day,mnth,year);
}

const char * DTtype::getMonthStr(U1_byte month){
	switch (month){
	case 0: return "";
	case 1: return "Jan";
	case 2: return "Feb";
	case 3: return "Mar";
	case 4: return "Apr";
	case 5: return "May";
	case 6: return "Jun";
	case 7: return "Jul";
	case 8: return "Aug";
	case 9: return "Sep";
	case 10: return "Oct";
	case 11: return "Nov";
	default: return "Dec";
	}
}

const char * DTtype::getDayStr(U1_byte day){
	switch (day){
	case 0: return "Mon";
	case 1: return "Tue";
	case 2: return "Wed";
	case 3: return "Thu";
	case 4: return "Fri";
	case 5: return "Sat";
	default: return "Sun";
	}
}

DTtype DTtype::getDateTime(U1_byte index) const { // returns datevalue as yyyyyymmmmddddddhhhhhmmm
	if (index == 255) 
		return JUDGEMEMT_DAY;
	return (*this & dateTimeMask);
}

DTtype DTtype::getTime() const {
	return (*this & timeMask);
}

DTtype DTtype::nextDay(DTtype date){
	date.day++;
	if (date.day > daysInMnth(date.mnth,date.year)) {
		date.mnth++;
		date.day = 1;
	}	
	if (date.mnth > 12) {
		date.year++;
		date.mnth = 1;
	}
	return date;
}

U1_byte DTtype::daysInMnth(U1_byte month, U1_byte year){ // static
	if (year == 128) { // use this / next year
		year = currTime().getDTyr();
		if (month < currTime().getDTmnth()) year ++;
	}
	switch (month) {
	case 2:return (year % 4) == 0 ? 29 :28;
	case 4:
	case 6:
	case 9:
	case 11:return 30;
	default: return 31;
	}
}

void DTtype::setDate(DTtype date){
	* this = (*this & ~dateMask) | (date & dateMask);
}

void DTtype::setTime(DTtype date) {
	* this = (*this & ~timeMask) | (date & timeMask);
}

void DTtype::setDateTime(DTtype dateTime){
	* this = ((*this & ~dateTimeMask) | (dateTime & dateTimeMask));
}

void DTtype::setDayMnthYr(U1_byte yr, U1_byte mn, U1_byte dayNo) {
	year = yr; 
	mnth = mn; 
	day = dayNo;
}

void DTtype::setHrMin(U1_byte hr, U1_byte min) {
	hrs = hr;
	mins = min/10;
	minUnits(min%10);
}

U1_byte DTtype::minUnits() const {
	return DPIndex & 15;
}

void DTtype::minUnits(U1_byte newMins) {
	DPIndex = (DPIndex & ~15) | newMins;
}

DTtype DTtype::addDateTime(int additionalTime) const { 
	// adds or subtracts time as hrs * 8 + 10's mins etc.
	// Hrs Range is 0-23. 24 is midnight and is set back to 0.
	DTtype newDate(* this);
	S1_byte newMins = newDate.mins + additionalTime % 8;
	S2_byte newHours = newDate.hrs + additionalTime / 8;
	S1_byte newDay = newDate.day;// + newHours / 24;
	//newHours = newHours % 24;
	S1_byte newMnth = newDate.mnth;

	if (newMins >= 6) {newMins -= 6; ++newHours;
	} else if (newMins < 0) {newMins += 6; --newHours;}
	newDate.mins = newMins;
	 
	//newDay += (newHours / 24) -1;
	while (newHours > 24) {newHours -= 24; ++newDay;}
	while (newHours < 0) {newHours += 24; --newDay;}
	if (newHours == 24 && newDate.hrs < 24) ++newDay; // roll over at midnight
	if (newHours == 23 && newDate.hrs == 0) --newDay; // roll back at midnight
	if (newHours == 24) newHours = 0;
	newDate.hrs = newHours;	

	if (newMnth > 12) {newMnth = 1; ++newDate.year;}
	U1_byte daysThisMonth = daysInMnth(newMnth, newDate.year);
	if (newDay > daysThisMonth) {
		++newMnth;
		if (newMnth > 12) {newMnth = 1; ++newDate.year;}
		newDay -= daysThisMonth;
	} else if (newDate.day > 0 && newDay < 1) {
		--newMnth;
		if (newMnth < 1) {newMnth = 12; --newDate.year;}
		newDay += daysInMnth(newMnth, newDate.year);
	}
	newDate.day = newDay;
	newDate.mnth = newMnth;
	return newDate;
}

DTtype DTtype::addDateTimeNoCarry(S4_byte additionalTime) { // adds +- time as hrs * 8 + 10's mins etc.
	bool wasNow = (getDateTime() == currentTime(true));
	static DTtype wasDateTime(currentTime());
	if (!wasNow) 
		wasDateTime = getDateTime();
	// if coming in at now and subtracting time, restore date previous to becoming now.
	// to acheive this we need to remember dateTime Unless becoming now.
	if (wasNow && additionalTime < 0) 
		setDateTime(wasDateTime);

	S1_byte addMins = additionalTime % 8;
	mins = nextIndex(0,mins, 5, addMins);
	S1_byte addHrs = (additionalTime % 256) /8;
	hrs = nextIndex(0,hrs, 23,addHrs);
	S1_byte addDay = (additionalTime /= 256) % 32;
	day = nextIndex(1,day,daysInMnth(mnth,year),addDay );
	S1_byte addMnth = (additionalTime / 32) % 16;
	mnth = nextIndex(1,mnth,12,addMnth);

	if (addMins && getDateTime() < currentTime()) {
		day = nextIndex(1,day,daysInMnth(mnth,year),S1_byte(1)); 
		addDay = 1;
	}
	if (!wasNow && (addHrs < 0 || addDay < 0 || addMnth < 0) && (getDateTime() < currentTime())) {
		setDateTime(currentTime()); 
		//wasNow = true;
	}
	if (addHrs && getDateTime() < currentTime()) {
		day = nextIndex(1,day,daysInMnth(mnth,year),S1_byte(1)); 
		addDay = 1;
	}
	//if (addDay && (getDateTime() < currentTime())) {mnth = nextIndex(1,mnth,12,1); addMnth = 1;}
	if ((addDay || addMnth) && (getDateTime() < currentTime())) year++;
	if (timeDiff(currentTime(),getDateTime()).year >= 1) year--;
	if (day > daysInMnth(mnth,year)) additionalTime > 0 ? day = 1 : day = daysInMnth(mnth,year);

	return *this;
}

//void DTtype::addMins(S1_byte add){}
//void DTtype::addHrs(S1_byte add){}
void DTtype::addDay(int add) {
	*this = addDateTime(add * 8 * 24);
}
//void DTtype::addMnth(S1_byte add){}
//void DTtype::addYr(S1_byte add){}

DTtype DTtype::timeDiff(DTtype startTime, DTtype endTime){ // time/date difference in days/hrs/mins.
	short timeDiff = endTime.mins - startTime.mins;
	if (timeDiff <0) {endTime.hrs--; timeDiff += 6;}
	endTime.mins = timeDiff;

	timeDiff = endTime.hrs - startTime.hrs;
	if (timeDiff <0) {endTime.day--; timeDiff += 24;}
	endTime.hrs = timeDiff;

	timeDiff = endTime.day - startTime.day;
	if (timeDiff <0) {endTime.mnth--; timeDiff += daysInMnth(startTime.mnth,startTime.year);}
	endTime.day = timeDiff;

	timeDiff = endTime.mnth - startTime.mnth;
	if (timeDiff <0) {endTime.year--; timeDiff += 12;}
	endTime.mnth = timeDiff;
	endTime.year = (endTime.year - startTime.year);
	return endTime;
}

void DTtype::loadDT(U2_epAdrr addr) {
	DTtype dtData(getLong(addr));
	mins = dtData.mins;
	hrs = dtData.hrs;
	day = dtData.day;
	mnth = dtData.mnth;
	year = dtData.year;
	DPIndex = dtData.DPIndex;	
}


void DTtype::saveDT(U2_epAdrr addr) {
	setLong(addr, this->U4());
}

