#include "debgSwitch.h"
#include "EEPROM_Utilities.h"

#if defined (ZPSIM)
	#include <iostream>
#endif

namespace EEPROM_Utilities {
////////////////////////////////////////////////////////////
//////////// Access data functions /////////////////////////
////////////////////////////////////////////////////////////
short EEPROMtop = EEPROMsize;
char romBlock[21];

char *getNTstring(U2_epAdrr start) { // null-terminated string
	U1_byte i = 0;
	do  {
		romBlock[i] = EEPROM->read(start + i);
	} while (romBlock[i++] != 0);
	return romBlock;
}

void setNTstring(U2_epAdrr start, const char *myString) { // null-terminated string
	U1_byte i = 0;
	do  {
		EEPROM->write(start + i,myString[i]);
	} while (myString[i++] != 0);
}

char *getString(U2_epAdrr start, U1_byte length){ // strings are saved without terminating null. length excludes null.
	for (U1_byte i = 0; i < length; i++) {
		romBlock[i] = EEPROM->read(start + i);
	}
	romBlock[length] = 0;
	return romBlock;
}

void setString(U2_epAdrr start,const char *myString, U1_byte length){ // for use with non-string data.
	for (U1_byte i = 0; i < length; i++) {
		EEPROM->write(start + i,myString[i]);
	}
}

void moveDataOut(char *destination, U2_epAdrr start, U1_byte length){
	for (U1_byte i = 0; i < length; i++) {
		destination[i] = EEPROM->read(start + i);
	}
}

U2_byte getShort(U2_epAdrr start){
	U2_byte i = EEPROM->read(start);
	i <<= 8;
	i = i + EEPROM->read(start+1);
	return i;
}

void setShort(U2_epAdrr start, U2_byte val){EEPROM->write(start,val >> 8); EEPROM->write(start +1,val & 255);}

U4_byte getLong(U2_epAdrr start){
	U4_byte i = 0;
	for (U1_byte j = 0; j<=3; j++){
		i <<= 8;
		i = i + EEPROM->read(start + j);
	}
	return i;
}

void setLong(U2_epAdrr start, U4_byte val){
	U1_byte i = 0;
	for (S1_byte j = 3; j>=0; j--){
		i = (U1_byte) val & 255;
		EEPROM->write(start + j,i);
		val >>= 8;
	}
}

void copyWithinEEPROM(U2_epAdrr sourceStart, U2_epAdrr destinationStart, U2_byte bytesToCopy){
	short begin;
	short end;
	short direction;
	short j;
	if (destinationStart < sourceStart) {
		begin = sourceStart;
		end = sourceStart + bytesToCopy;
		direction = 1;
		j = destinationStart;
	} else {
		begin = sourceStart + bytesToCopy - 1;
		end = sourceStart - 1;
		direction = -1;
		j = destinationStart + bytesToCopy - 1;
	}

//std::cout << "From : To\n"; bytesToCopy+=2;
	for (short i = begin; i != end; i += direction) {
//std::cout << int(i) << " : " << int(j) << " : " << int(--bytesToCopy/2) << " : " << (int)EEPROM->read(i) << '\n';
		EEPROM->write(j,EEPROM->read(i));
		j += direction;
	}
}

void swapData(U2_epAdrr start1, U2_epAdrr start2, U2_byte length){
	short end = start1 + length;
	short j = start2;
	U1_byte temp;
	for (short i = start1; i < end; i++){
		temp = EEPROM->read(j);
		EEPROM->write(j,EEPROM->read(i));
		EEPROM->write(i,temp);
		j= j+1;
	}
}

#if defined (ZPSIM)
void debugEEprom(U2_epAdrr start, U1_byte length) {
	std::cout << "\n";
	for (short i = start; i < start + length; i++) {
		std::cout << (int)EEPROM->read(i) << "\t" << (int)EEPROM->read(i) << "\n"; 
	}
}
#endif
}