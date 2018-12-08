#pragma once
#include <Arduino.h>

///////////////////////////// DateTime /////////////////////////////////////
// Single Responsibility is to define common interface for DateTime classes //
//////////////////////////////////////////////////////////////////////////////

namespace GP_LIB {
	class DTtype;

	bool operator==(const DTtype& lhs, const DTtype& rhs);
	inline bool operator==(const uint8_t& lhs, const DTtype& rhs) { return operator==((const DTtype&)lhs, rhs); }
	inline bool operator==(const DTtype& lhs, const uint8_t& rhs) { return operator==(lhs, (const DTtype&)rhs); }
	bool operator< (const DTtype& lhs, const DTtype& rhs);
	inline bool operator!=(const DTtype& lhs, const DTtype& rhs) { return !operator==(lhs, rhs); }
	inline bool operator> (const DTtype& lhs, const DTtype& rhs) { return  operator< (rhs, lhs); }
	inline bool operator<=(const DTtype& lhs, const DTtype& rhs) { return !operator> (lhs, rhs); }
	inline bool operator>=(const DTtype& lhs, const DTtype& rhs) { return !operator< (lhs, rhs); }
	DTtype operator& (const DTtype& lhs, const DTtype& rhs);
	DTtype operator| (const DTtype& lhs, const DTtype& rhs);

	class DTtype {
	public:
		// constructors
		DTtype();
		explicit DTtype(uint8_t z);
		explicit DTtype(uint32_t longDT); // to suppoprt comparason operators
		//explicit DTtype(int32_t longDT);
		DTtype(uint8_t yy, uint8_t mm, uint8_t dd, uint8_t hh, uint8_t n10);

		// queries
		DTtype operator~ () const;
		operator int16_t() const; // converions to S2_Byte
		uint8_t U1() const; // converions to uint8_t
		//int32_t S4() const; // converions to int32_t
		uint32_t U4() const; // converions to uint32_t

		DTtype getDate() const;
		DTtype getTime() const;
		uint8_t getDayNoOfDate() const;
		DTtype getDateTime(uint8_t index = 0) const;
		uint8_t get10Mins() const { return mins10; }
		uint8_t getHrs() const { return hrs; }
		uint8_t getDay() const { return day; }
		uint8_t getMnth() const { return mnth; }
		uint8_t getYr() const { return year; }
		//uint8_t getDPindex() const { return DPIndex; }
		DTtype addDateTime(int additionalTime) const; // // adds or subtracts time as hrs * 8 + 10's mins. returns offset date

		// modifiers
		void setDate(DTtype date);
		void setTime(DTtype date);
		void setTime(int hr, int mins10);
		void setDateTime(DTtype dateTime);
		void setDayMnthYr(uint8_t yr, uint8_t mn, uint8_t dayNo);
		//void saveDT(U2_EpAddr addr);
		//void loadDT(U2_EpAddr addr);

		//void addMins(S1_Byte add);
		//void addHrs(S1_Byte add);
		void addDay(int add);
		//void addMnth(S1_Byte add);
		//void addYr(S1_Byte add);
		DTtype addDateTimeNoCarry(int32_t additionalTime, DTtype now = DTtype());

		// statics
		static DTtype nextDay(DTtype date);
		static DTtype timeDiff(DTtype startTime, DTtype endTime); // returns time difference in hours(*8) & mins(10's)
		static const DTtype MAX_PERIOD;
		static const DTtype JUDGEMEMT_DAY;
		static const DTtype dateTimeMask;
		static const DTtype MIDNIGHT;
		//static std::string getTimeDateStr(DTtype dt);
		//static const char * getMonthStr(uint8_t month);
		//static const char * getDayStr(uint8_t day);
		static uint8_t displayHrs(uint8_t hrs);
	protected:
		static const DTtype dateMask;
		static const DTtype timeMask;
		

		unsigned mins10 : 3; // 10's of mins
		unsigned hrs : 5; // Time 8 bits. Hrs range 0-23. 0hrs is Midnight, displayed as 12pm
		unsigned day : 5;
		unsigned mnth : 4;
		unsigned year : 7;  // Date 16 bits


	private:
		operator int32_t() const = delete; // prevent implicit convertions to int32_t
	};

	namespace dateUtils {
		uint8_t getDayNoOfDate(uint8_t day, uint8_t mnth, uint16_t yy);
		const char * getMonthStr(uint8_t month);
		const char * getDayStr(uint8_t day);
		uint8_t daysInMnth(uint8_t month, uint8_t year, DTtype now = DTtype());
		uint8_t displayHrs(uint8_t hrs);
	}
}