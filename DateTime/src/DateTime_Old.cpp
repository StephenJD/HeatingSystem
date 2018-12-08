#include "DateTime_Old.h"
#include "Convertions.h"

namespace GP_LIB {

	// Global binary operators //
	bool operator== (const DTtype& lhs, const DTtype& rhs) {
		return ((lhs & DTtype::dateTimeMask).U4() == (rhs & DTtype::dateTimeMask).U4());
	}

	bool operator< (const DTtype& lhs, const DTtype& rhs) {
		return ((lhs & DTtype::dateTimeMask).U4() < (rhs & DTtype::dateTimeMask).U4());
	}

	DTtype operator& (const DTtype& lhs, const DTtype& rhs) {
		return DTtype(lhs.U4() & rhs.U4());
	}

	DTtype operator| (const DTtype& lhs, const DTtype& rhs) {
		return DTtype(lhs.U4() | rhs.U4());
	}

	// Constructors //
	DTtype::DTtype(uint32_t longDT) { // DTtype constructor taking a long for loading from EEPROM.
		const void * vPtr = &longDT; // void pointer at longDT
		*this = *(reinterpret_cast<const DTtype*>(vPtr)); // cast void pointer as pointer to DTtype and assign. 
	}

	//DTtype::DTtype(int32_t longDT) { // DTtype constructor taking a long for loading from EEPROM.
	//	const void * vPtr = &longDT; // void pointer at longDT
	//	*this = *(reinterpret_cast<const DTtype*>(vPtr)); // cast void pointer as pointer to DTtype and assign. 
	//}
	DTtype::DTtype() : mins10(0), hrs(0), day(0), mnth(0), year(0) {}
	DTtype::DTtype(uint8_t z) : mins10(z & 7), hrs(z / 8), day(0), mnth(0), year(0){ if (z == 255) { day = 31; mnth = 15; year = 127; } };
	DTtype::DTtype(uint8_t yy, uint8_t mm, uint8_t dd, uint8_t hh, uint8_t n10) : mins10(n10), hrs(hh), day(dd), mnth(mm), year(yy) {};

	// operators //

	DTtype DTtype::operator~ () const {
		return DTtype(~this->U4());
	}

	uint32_t DTtype::U4() const {
		const void * longDT = this; // void pointer at this
		return *(reinterpret_cast<const uint32_t *>(longDT)); // cast void pointer as pointer to long. 
	}

	//int32_t DTtype::S4() const {
	//	const void * longDT = this; // void pointer at this
	//	return *(reinterpret_cast<const int32_t *>(longDT)); // cast void pointer as pointer to long. 
	//}

	DTtype::operator int16_t() const { return this->mins10 + this->hrs * 8 + this->day * 192; }

	uint8_t DTtype::U1() const { return this->mins10 + this->hrs * 8 + this->day * 192; }

	const DTtype DTtype::dateTimeMask(127, 15, 31, 31, 7);
	const DTtype DTtype::dateMask(127, 15, 31, 0, 0);
	const DTtype DTtype::timeMask(0, 0, 0, 31, 7);
	const DTtype DTtype::JUDGEMEMT_DAY(127, 15, 31, 0, 0);
	const DTtype DTtype::MAX_PERIOD(0, 0, 1, 7, 5);
	const DTtype DTtype::MIDNIGHT(0, 0, 0, 24, 0);

	DTtype DTtype::getDate() const {
		return *this & dateMask;
	}

	uint8_t DTtype::getDayNoOfDate() const { // returns 0-6 with 0 == Monday.
		return dateUtils::getDayNoOfDate(day, mnth, year);
	}

	DTtype DTtype::getDateTime(uint8_t index) const { // returns datevalue as yyyyyymmmmddddddhhhhhmmm
		if (index == 255)
			return JUDGEMEMT_DAY;
		return (*this & dateTimeMask);
	}

	DTtype DTtype::getTime() const {
		return (*this & timeMask);
	}

	DTtype DTtype::nextDay(DTtype date) {
		date.day++;
		if (date.day == 0 || date.day > dateUtils::daysInMnth(date.mnth, date.year)) {
			date.mnth++;
			date.day = 1;
		}
		if (date.mnth > 12) {
			date.year++;
			date.mnth = 1;
		}
		return date;
	}

	void DTtype::setDate(DTtype date) {
		*this = (*this & ~dateMask) | (date & dateMask);
	}

	void DTtype::setTime(DTtype date) {
		*this = (*this & ~timeMask) | (date & timeMask);
	}

	void DTtype::setTime(int hr, int mins10) {
		DTtype time;
		time.hrs = hr;
		time.mins10 = mins10;
		setTime(time);
	}


	void DTtype::setDateTime(DTtype dateTime) {
		*this = ((*this & ~dateTimeMask) | (dateTime & dateTimeMask));
	}

	void DTtype::setDayMnthYr(uint8_t yr, uint8_t mn, uint8_t dayNo) {
		year = yr;
		mnth = mn;
		day = dayNo;
	}

	DTtype DTtype::addDateTime(int additionalTime) const {
		// adds or subtracts time as hrs * 8 + 10's mins etc.
		// Hrs Range is 0-23. 24 is midnight and is set back to 0.
		DTtype newDate(*this);
		int8_t newMins = newDate.mins10 + additionalTime % 8;
		int16_t newHours = newDate.hrs + (additionalTime % 256) / 8;
		int8_t newDay = newDate.day + (additionalTime /= 256) % 32;
		int8_t newMnth = newDate.mnth + (additionalTime / 32) % 16;

		if (newMins >= 6) {
			newMins -= 6; ++newHours;
		}
		else if (newMins < 0) { newMins += 6; --newHours; }
		newDate.mins10 = newMins;

		//newDay += (newHours / 24) -1;
		while (newHours > 24) { newHours -= 24; ++newDay; }
		while (newHours < 0) { newHours += 24; --newDay; }
		if (newHours == 24 && newDate.hrs < 24) ++newDay; // roll over at midnight
		if (newHours == 23 && newDate.hrs == 0) --newDay; // roll back at midnight
		if (newHours == 24) newHours = 0;
		newDate.hrs = newHours;

		if (newMnth > 12) { newMnth = 1; ++newDate.year; }
		uint8_t daysThisMonth = dateUtils::daysInMnth(newMnth, newDate.year);
		if (newDay > daysThisMonth) {
			++newMnth;
			if (newMnth > 12) { newMnth = 1; ++newDate.year; }
			newDay -= daysThisMonth;
		}
		else if (newDate.day > 0 && newDay < 1) {
			--newMnth;
			if (newMnth < 1) { newMnth = 12; --newDate.year; }
			newDay += dateUtils::daysInMnth(newMnth, newDate.year);
		}
		newDate.day = newDay;
		newDate.mnth = newMnth;
		return newDate;
	}

	DTtype DTtype::addDateTimeNoCarry(int32_t additionalTime, DTtype now) { // adds +- time-date as hrs * 8 + 10's mins etc.
		bool wasNow = (getDateTime() == now);
		static DTtype wasDateTime(now);
		if (!wasNow)
			wasDateTime = getDateTime();
		// if coming in at now and subtracting time, restore date previous to becoming now.
		// to acheive this we need to remember dateTime Unless becoming now.
		if (wasNow && additionalTime < 0)
			setDateTime(wasDateTime);

		int8_t addMins = additionalTime % 8;
		mins10 = nextIndex(0, mins10, 5, addMins);
		int8_t addHrs = (additionalTime % 256) / 8;
		hrs = nextIndex(0, hrs, 23, addHrs);
		int8_t addDay = (additionalTime /= 256) % 32;
		day = nextIndex(1, day, dateUtils::daysInMnth(mnth, year), addDay);
		int8_t addMnth = (additionalTime / 32) % 16;
		mnth = nextIndex(1, mnth, 12, addMnth);

		if (addMins && getDateTime() < now) {
			day = nextIndex(1, day, dateUtils::daysInMnth(mnth, year), int8_t(1));
			addDay = 1;
		}
		if (!wasNow && (addHrs < 0 || addDay < 0 || addMnth < 0) && (getDateTime() < now)) {
			setDateTime(now);
		}
		if (addHrs && getDateTime() < now) {
			day = nextIndex(1, day, dateUtils::daysInMnth(mnth, year), int8_t(1));
			addDay = 1;
		}
		if ((addDay || addMnth) && (getDateTime() < now)) year++;
		if (timeDiff(now, getDateTime()).year >= 1) year--;
		if (day > dateUtils::daysInMnth(mnth, year)) additionalTime > 0 ? day = 1 : day = dateUtils::daysInMnth(mnth, year);

		return *this;
	}

	//void DTtype::addMins(int8_t add){}
	//void DTtype::addHrs(int8_t add){}
	void DTtype::addDay(int add) {
		*this = addDateTime(add * 8 * 24);
	}
	//void DTtype::addMnth(int8_t add){}
	//void DTtype::addYr(int8_t add){}

	DTtype DTtype::timeDiff(DTtype startTime, DTtype endTime) { // time/date difference in days/hrs/mins10.
		short timeDiff = endTime.mins10 - startTime.mins10;
		if (timeDiff < 0) { endTime.hrs--; timeDiff += 6; }
		endTime.mins10 = timeDiff;

		timeDiff = endTime.hrs - startTime.hrs;
		if (timeDiff < 0) { endTime.day--; timeDiff += 24; }
		endTime.hrs = timeDiff;

		timeDiff = endTime.day - startTime.day;
		if (timeDiff < 0) { endTime.mnth--; timeDiff += dateUtils::daysInMnth(startTime.mnth, startTime.year); }
		endTime.day = timeDiff;

		timeDiff = endTime.mnth - startTime.mnth;
		if (timeDiff < 0) { endTime.year--; timeDiff += 12; }
		endTime.mnth = timeDiff;
		endTime.year = (endTime.year - startTime.year);
		return endTime;
	}

	//std::string DTtype::getTimeDateStr(DTtype dt) {
	//	char fieldText[21];
	//	if (currentTime(true) >= dt) {
	//		strcpy(fieldText, "Now");
	//	}
	//	else {
	//		strcpy(fieldText, intToString(dateUtils::displayHrs(dt.getHrs()), 2, '0'));
	//		strcat(fieldText, ":");
	//		strcat(fieldText, intToString(dt.get10Mins() * 10, 2, '0'));
	//		strcat(fieldText, (dt.getHrs() < 12) ? "am" : "pm");
	//		DTtype gotDate = dt.getDate();
	//		if (gotDate == currentTime().getDate()) {
	//			strcat(fieldText, " Today");
	//		}
	//		else if (gotDate == DTtype::nextDay(currentTime().getDate())) {
	//			strcat(fieldText, " Tomor'w");
	//		}
	//		else if (gotDate == DTtype::JUDGEMEMT_DAY) {
	//			strcpy(fieldText, "Judgement Day");
	//		}
	//		else {
	//			strcat(fieldText, " ");
	//			strcat(fieldText, intToString(dt.getDay(), 2, '0'));
	//			strcat(fieldText, dateUtils::getMonthStr(dt.getMnth()));
	//		}
	//		if (dt.getYr() != 127 && dt.getYr() > currentTime().getYr()) strcat(fieldText, intToString(dt.getYr(), 2, '0'));
	//	}
	//	return fieldText;
	//}

	uint8_t DTtype::displayHrs(uint8_t hrs) {
		if (hrs > 12) hrs -= 12;
		if (hrs == 0) hrs = 12;
		return hrs;
	}

	//const char * DTtype::getMonthStr(uint8_t month) { // static
	//	switch (month) {
	//	case 0: return "";
	//	case 1: return "Jan";
	//	case 2: return "Feb";
	//	case 3: return "Mar";
	//	case 4: return "Apr";
	//	case 5: return "May";
	//	case 6: return "Jun";
	//	case 7: return "Jul";
	//	case 8: return "Aug";
	//	case 9: return "Sep";
	//	case 10: return "Oct";
	//	case 11: return "Nov";
	//	default: return "Dec";
	//	}
	//}

	//const char * DTtype::getDayStr(uint8_t day) { // static
	//	switch (day) {
	//	case 0: return "Mon";
	//	case 1: return "Tue";
	//	case 2: return "Wed";
	//	case 3: return "Thu";
	//	case 4: return "Fri";
	//	case 5: return "Sat";
	//	default: return "Sun";
	//	}
	//}

	//void DTtype::loadDT(U2_EpAddr addr) {
	//	DTtype dtData(getLong(addr));
	//	mins10 = dtData.mins10;
	//	hrs = dtData.hrs;
	//	day = dtData.day;
	//	mnth = dtData.mnth;
	//	year = dtData.year;
	//	DPIndex = dtData.DPIndex;
	//}
	//
	//
	//void DTtype::saveDT(U2_EpAddr addr) {
	//	setLong(addr, this->U4());
	//}

	namespace dateUtils {
		uint8_t daysInMnth(uint8_t month, uint8_t year, DTtype now) { // static
			if (year == 128) { // use this / next year
				year = now.getYr();
				if (month < now.getMnth()) year++;
			}
			switch (month) {
			case 2:return (year % 4) == 0 ? 29 : 28;
			case 4:
			case 6:
			case 9:
			case 11:return 30;
			default: return 31;
			}
		}

		uint8_t displayHrs(uint8_t hrs) {
			if (hrs > 12) hrs -= 12;
			if (hrs == 0) hrs = 12;
			return hrs;
		}

		uint8_t getDayNoOfDate(uint8_t day, uint8_t month, uint16_t yy) { // static
																				 // returns day number. 0 = Monday
			yy = yy + 2000;
			if (month < 3) {
				month += 12;
				--yy;
			}

			return (--day + (2 * month) + (6 * (month + 1) / 10) +
				yy + (yy / 4) - (yy / 100) +
				(yy / 400) + 1) % 7;
		}

		const char * getMonthStr(uint8_t month) {
			switch (month) {
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

		const char * getDayStr(uint8_t day) {
			switch (day) {
			case 0: return "Mon";
			case 1: return "Tue";
			case 2: return "Wed";
			case 3: return "Thu";
			case 4: return "Fri";
			case 5: return "Sat";
			default: return "Sun";
			}
		}
	}
}