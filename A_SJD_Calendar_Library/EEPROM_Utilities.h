#if !defined (_EEPROM_Utilities_H_)
#define _EEPROM_Utilities_H_

#include "debgSwitch.h"
#include "A__Constants.h"
#include <EEPROM.h>

namespace EEPROM_Utilities
{
	extern short EEPROMtop;
	extern char romBlock[21];

	// Access data functions
	void setString(U2_epAdrr start,const char *myString, U1_byte length);
	char *getString(U2_epAdrr start, U1_byte length);
	void setNTstring(U2_epAdrr start,const char *myString); // null-terminated string
	char *getNTstring(U2_epAdrr start); // null-terminated string
	void setShort(U2_epAdrr start, U2_byte val);
	U2_byte getShort(U2_epAdrr start);
	void moveDataOut(char *destination, U2_epAdrr start, U1_byte length);
	U4_byte getLong(U2_epAdrr start);
	void setLong(U2_epAdrr start, U4_byte val);
	void copyWithinEEPROM(U2_epAdrr sourceStart, U2_epAdrr destinationStart, U2_byte length);
	void swapData(U2_epAdrr start1, U2_epAdrr start2, U2_byte length);
	#if defined (ZPSIM)
		void debugEEprom(U2_epAdrr start, U1_byte length);
	#endif
}
#endif