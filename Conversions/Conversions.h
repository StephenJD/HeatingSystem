#pragma once

#ifdef ZPSIM
#include <cstring>
#else
#include <string.h>
#endif


//#define DISPLAY_BUFFER_LENGTH 40
//#define MAX_LINE_LENGTH 20

extern char SPACE_CHAR;
extern const char NEW_LINE_CHR;

namespace GP_LIB {
	enum Format : unsigned char { e_edOneShort, e_editAll = 1, e_showSign = 2, e_fixedWidth = 4, e_inEdit = 8 };
	
	int nextIndex(int minVal, int currVal, int maxVal, int move, bool carryOver = true);

	/// <summary>
	/// Short Copyable c-string
	/// </summary>
	class CStr_10 {
	public:
		CStr_10(const char * str) { strncpy(_str, str, 10); }
		CStr_10() { _str[0] = 0; }
		CStr_10 & operator=(const char * str) { strncpy(_str, str, 10); return *this; }
		char & operator[](int index) { return _str[index]; }
		char operator[](int index) const { return _str[index]; }
		operator const char *() const { return _str; }
		operator char *() { return _str; }
	private:
		char _str[10];
	};
	
	/// <summary>
	/// Returns number of significant digits
	/// </summary>
	int significantDigits(int val);

	/// <summary>
	/// only for values &lt; 1029
	/// </summary>
	inline int div10(int val) { // only valid for values < 1029
		return (val * 205) >> 11;
	}
	
	/// <summary>
	/// only for values &lt; 1029
	/// </summary>
	inline int mod10(int val) { // only valid for values < 1029
		return val - (div10(val) * 10);
	}

	/// <summary>
	/// only for values &lt; 1029
	/// </summary>
	CStr_10 intToString(int value);
	
	/// <summary>
	/// only for values &lt; 1029
	/// If minNoOfChars -ve, then add + prefix.
	/// </summary>
	CStr_10 intToString(int value, int minNoOfChars, char leadingChar = '0', int format = e_editAll | e_fixedWidth);
	
	/// <summary>
	/// only for int values &lt; 1029
	/// If minNoOfChars -ve, then add + prefix.
	/// abs(minNoOfChars) must be at least 1 more than noOfDecPlaces
	/// </summary>
	CStr_10 decToString(int number, int minNoOfChars, int noOfDecPlaces, int format = e_editAll | e_fixedWidth);
	
	/// <summary>
	/// Returns 10^power up to 3 (1000)
	/// </summary>
	int power10(unsigned int power); // Returns 10^power
	
	/// <summary>
	/// Converts two integer characters to their integer equivalent
	/// No checks made!
	/// </summary>
	inline unsigned char c2CharsToInt(const char* p) {
		unsigned char v = 0;
		if ('0' <= *p && *p <= '9')
			v = *p - '0';
		return 10 * v + *++p - '0';
	}

	/// <summary>
	/// Converts from binary to a two-digit BCD representation  
	/// </summary>
	/// <param name="value">An 8-bit binary number 0-99.</param>
	/// <returns></returns>
	inline unsigned char toBCD(unsigned char value) {
		return ((div10(value)) << 4) + (mod10(value));
	}

	/// <summary>
	/// Converts from a two-digit BCD representation to binary
	/// </summary>
	/// <param name="value">A two-digit BCD value</param>
	/// <returns></returns>
	inline unsigned char fromBCD(unsigned char value) {
		unsigned char decoded = value & 127;
		return (decoded & 15) + 10 * ((decoded & (15 << 4)) >> 4);
	}

}