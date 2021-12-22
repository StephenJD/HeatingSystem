#include "Conversions.h"
#include "Arduino.h"

namespace GP_LIB {
	using namespace std;

	char SPACE_CHAR = '~';
	const char NEW_LINE_CHR = '~';

	// *************************************************************
	// ***************  Utility ************************************

	int nextIndex(int minVal, int currVal, int maxVal, int move, bool carryOver) { // return index, which has max value of noOfMembers -1
																				   // when incrementing by 10 or 100, leave the lower digits unchanged when overflowing the max and min values.
		int noOfMembers = maxVal - minVal + 1;
		if (noOfMembers <= 0) return 0;
		int absMove = move >= 0 ? move : -move;
		int power = 1;
		if (!carryOver) while (absMove / (power * 10) >= 1) power = power * 10;

		int tempVal = currVal / power - minVal / power; // currentVal relative to minVal

		int tempNoOfMembers = (noOfMembers - 1) / power + 1;
		int sum = tempVal + move / power; // new value relative to minVal
		if (sum < 0) sum = sum + tempNoOfMembers; // cycle to end
		if (sum >= tempNoOfMembers && tempNoOfMembers >= 10) sum = sum - tempNoOfMembers; else sum = sum % tempNoOfMembers;
		sum = (minVal / power + sum) * power + (currVal % power);
		if (sum >= minVal + noOfMembers) sum = minVal + noOfMembers - 1;
		if (sum < minVal) sum = minVal;
		return sum;
	}

	int significantDigits(int value) {
		value = abs(value);
		return (value < 10 ? 1 :
			(value < 100 ? 2 : 
			(value < 1000 ? 3 : 4)));			
	}

	CStr_20 intToString(int value) { // only valid for values < 1029
		// TODO: implement using arduino Print / PString
		//if (abs(value) > 1028)
		//	value = 1028;
		auto cstr_20 = CStr_20{};
		char * p = cstr_20.str();
		if (value < 0) {
			*p = '-';
			++p;
			value = -value;
		}
		p += significantDigits(value);
		*p = '\0';
		do {
			*--p = "0123456789"[mod10(value)];
		} while (value = div10(value));
		return cstr_20;
	}

	int power10(unsigned int power) {
		static int powers10[] = { 1, 10, 100, 1000 };
		if (power < sizeof(powers10) / sizeof(int)) {
			return powers10[power];
		}
		else return 1;
	}

	CStr_20 intToString(int value, int minNoOfChars, char leadingChar, int format) {
		bool leadPlus = (format & e_showSign) != 0;
		if ((format & e_fixedWidth) == 0) minNoOfChars = 0;
		//bool inEdit = (format & (e_inEdit | e_editAll)) == (e_inEdit | e_editAll);
		int charIndex = 0;

		auto numStr_20 = intToString(abs(value));
		int editablePlaces = static_cast<int>(strlen(numStr_20));

		auto padStr_10 = CStr_20{};

		int noOfpadding = 0;
		if (leadingChar == '0') {
			if (value >= 0) {
				if (leadPlus) {
					padStr_10[charIndex] = '+';
					++charIndex;
				}
			}
			else {
				padStr_10[charIndex] = '-';
				++charIndex;
			}
		}
		if (minNoOfChars > editablePlaces) noOfpadding = minNoOfChars - editablePlaces + charIndex;
		for (; charIndex < noOfpadding; ++charIndex) {
			padStr_10[charIndex] = leadingChar;
		} 
		if (leadingChar != '0') {
			if (value >= 0) {
				if (leadPlus) {
					padStr_10[charIndex] = '+';
					++charIndex;
				}
			} else {
				padStr_10[charIndex] = '-';
				++charIndex;
			}
		}		
		padStr_10[charIndex] = 0;
		strcat(padStr_10.str(), numStr_20);
		return padStr_10;
	}

	CStr_20 decToString(int number, int minNoOfChars, int noOfDecPlaces, int format) {
		// TODO: implement using arduino Print / PString
		auto padStr_10 = intToString(number, minNoOfChars,' ',format);

		int decPos = static_cast<int>(strlen(padStr_10)) - noOfDecPlaces - 1;
		if (padStr_10[decPos] < '0' || padStr_10[decPos] > '9') {
			// insert preceeding 0 if there is no digit to left of dec place.
			if (decPos -1 >= 0) padStr_10[decPos - 1] = padStr_10[decPos];
			padStr_10[decPos] = '0';
		}
		for (int i = 5; i > decPos; --i) {
			padStr_10[i+1] = padStr_10[i];
		}
		padStr_10[decPos + 1] = '.';
		return padStr_10;
	}
}