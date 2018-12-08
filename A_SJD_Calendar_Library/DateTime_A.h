#if !defined (_A_DATETIME_H_)
#define _A_DATETIME_H_

///////////////////////////// DateTime_A /////////////////////////////////////
// Single Responsibility is to define common interface for DateTime classes //
//////////////////////////////////////////////////////////////////////////////
#include "A__Constants.h"
#include "D_DataStream.h"
#include "D_EpData.h"

class DTtype;

bool operator==(const DTtype& lhs, const DTtype& rhs);
inline bool operator==(const U1_byte& lhs, const DTtype& rhs){return operator==((const DTtype&)lhs,rhs);}
inline bool operator==(const DTtype& lhs, const U1_byte& rhs){return operator==(lhs,(const DTtype&)rhs);}
bool operator< (const DTtype& lhs, const DTtype& rhs);
inline bool operator!=(const DTtype& lhs, const DTtype& rhs){return !operator==(lhs,rhs);}
inline bool operator> (const DTtype& lhs, const DTtype& rhs){return  operator< (rhs,lhs);}
inline bool operator<=(const DTtype& lhs, const DTtype& rhs){return !operator> (lhs,rhs);}
inline bool operator>=(const DTtype& lhs, const DTtype& rhs){return !operator< (lhs,rhs);}
DTtype operator& (const DTtype& lhs, const DTtype& rhs);
DTtype operator| (const DTtype& lhs, const DTtype& rhs);

class DTtype {
public:
	// constructors
	DTtype();
	DTtype(U1_byte z);
	DTtype(U1_byte y, U1_byte m, U1_byte d, U1_byte h, U1_byte n);
	DTtype(const char* date, const char* time);
	DTtype(U4_byte longDT);
	DTtype(S4_byte longDT);

	DTtype operator~ () const;
	operator int() const; // converions to S2_byte
	U1_byte U1() const; // converions to U1_byte
	S4_byte S4() const; // converions to S4_byte
	U4_byte U4() const; // converions to U4_byte

	// queries
	DTtype getDate() const;
	DTtype getTime() const;
	U1_byte getDayNoOfDate() const;
	DTtype getDateTime(U1_byte index = 0) const;
	U1_byte get10Mins() const {return mins;}
	U1_byte getHrs() const {return hrs;}
	U1_byte getDay() const {return day;}
	U1_byte getMnth() const {return mnth;}
	U1_byte getYr() const {return year;}
	U1_byte getDPindex() const {return DPIndex;}
	DTtype addDateTime(int additionalTime) const; // // adds or subtracts time as hrs * 8 + 10's mins. returns offset date
	U1_byte minUnits() const;

	// modifiers
	void setDate(DTtype date);
	void setTime(DTtype date);
	void setDateTime(DTtype dateTime);
	void setDayMnthYr(U1_byte yr, U1_byte mn, U1_byte dayNo);
	void setHrMin(U1_byte hr, U1_byte min);
	void saveDT(U2_epAdrr addr);
	void loadDT(U2_epAdrr addr);
	void minUnits(U1_byte);

	//void addMins(S1_byte add);
	//void addHrs(S1_byte add);
	void addDay(int add);
	//void addMnth(S1_byte add);
	//void addYr(S1_byte add);
	DTtype addDateTimeNoCarry(S4_byte additionalTime);

	// statics
	static U1_byte getDayNoOfDate(U1_byte day, U1_byte mnth, U2_byte yy);
	static const char * getMonthStr(U1_byte month);
	static const char * getDayStr(U1_byte day);
	static DTtype nextDay(DTtype date);
	static U1_byte daysInMnth(U1_byte month, U1_byte year);
	static DTtype timeDiff(DTtype startTime, DTtype endTime); // returns time difference in hours(*8) & mins(10's)
	static const DTtype MAX_PERIOD;
	static const DTtype JUDGEMEMT_DAY;
	static const DTtype dateTimeMask;
	static const DTtype MIDNIGHT;
	static U1_byte displayHrs(U1_byte hrs);
protected:
	friend class NoOf_EpD;
	static const DTtype dateMask;
	static const DTtype timeMask;

	unsigned mins: 3; // 10's of mins
	unsigned hrs: 5; // 8 bits. Same as TTtime. Range 0-23. 0hrs is Midnight, displayed as 12pm
	unsigned day: 5; 
	unsigned mnth: 4;
	unsigned year: 7; // yy 16 bits
	unsigned DPIndex: 8; // 4 bits double as minute units, autoDST(16) & dstHasBeenSet(32) for currentTime

private:
	uint8_t toBCD(uint8_t value) const;
	operator S4_byte() const; // prevent implicit convertions to S4_Byte
};

DTtype currentTime(bool set = false);

#endif