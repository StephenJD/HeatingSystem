#include "debgSwitch.h"
#include "D_EpDat_A.h"
#include "EEPROM_Utilities.h"
#include "A_Stream_Utilities.h"

#if defined (ZPSIM)
	#include <iostream>
#endif

using namespace EEPROM_Utilities;
using namespace Utils;
#include <EEPROM.h>

//EEPROMClass debug;

EpData::EpData() : ep_index(0) {}

const U2_epAdrr EpData::getDataStart() const {
	return getShort(getDataStartPtr());
}

void EpData::setDataStart(U2_epAdrr start){
	if (start != getShort(getDataStartPtr())){
		setShort(getDataStartPtr(),start);
	}
}

void EpData::setIndex(U1_ind myIndex) {
	ep_index = myIndex;
}

U1_ind EpData::index() const {return ep_index;}

void EpData::setName(const char * name) {
	setString(eepromAddr() + getNoOfVals(), name, getNameSize());
}

char * EpData::getName() const {
	if (hasNoName()) {
		return cIntToStr(ep_index,3,'0');
	} else {
		return getString(eepromAddr() + getNoOfVals(), getNameSize());
	}
}

void EpData::setVal(U1_ind vIndex, U1_byte val){
	EEPROM->write(eepromAddr() + vIndex, val);
}

U1_byte EpData::getVal(U1_ind vIndex) const {
	return EEPROM->read(eepromAddr() + vIndex);
}

U2_epAdrr EpData::eepromAddr() const {
	U2_epAdrr dataStart = getDataStart();
	U2_epAdrr dataSize =  getDataSize();
	return dataStart + dataSize * ep_index;
}

bool EpData::hasNoName() const {
	char firstChar = EEPROM->read(eepromAddr() + getNoOfVals());
	return firstChar == 0;
}

void EpData::move(U1_ind startPos, U1_ind destination, U1_count noToMove) {
	#if defined DEBUG_INFO
		std::cout << "Move within Data: SectionAddr: " << (int)getDataStart() << " DataSize: " << (int)getDataSize() << '\n';
		std::cout << " StartIndex: " << (int)startPos << " Dest Index:" << (int)destination << " NoToMove: " << (int)noToMove << '\n';
	#endif
	copyWithinEEPROM(getDataStart() + getDataSize() * startPos,
	getDataStart() + getDataSize() * destination,
	getDataSize() * noToMove);
}
