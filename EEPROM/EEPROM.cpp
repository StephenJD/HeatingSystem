/*
  EEPROM.cpp - EEPROM library
  Copyright (c) 2006 David A. Mellis.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/******************************************************************************
 * Includes
 ******************************************************************************/

#if defined(__SAM3X8E__) || defined ZPSIM

#include "EEPROM.h"
#include "I2C_Talk.h"
using namespace I2C_Talk_ErrorCodes;

/******************************************************************************
 * Constructors
 ******************************************************************************/
#if defined (ZPSIM)
	#include <iostream>
	#include <fstream>
	using namespace std;

	uint8_t EEPROMClass::myEEProm[HardwareInterfaces::EEPROM_SIZE];
	auto i2c = I2C_Talk();

	//EEPROMClass_T<i2c> fileEEPROM(0x50);
#endif	
	EEPROMClass::EEPROMClass(int addr) : I_I2Cdevice(addr) {
#if defined LOAD_EEPROM
		auto myfile = ifstream("EEPROM.dat", ios::binary);	// Input file stream
		if (myfile.is_open()) {
			myfile.read((char*)myEEProm, sizeof(myEEProm));
			myfile.close();
		}
		else cout << "Unable to open file\n";
		myfile = ifstream("I2C.dat", ios::binary);	// Input file stream
		if (myfile.is_open()) {
			myfile.read((char*)Wire1.i2CArr, sizeof(Wire1.i2CArr));
			myfile.close();
		}
		else cout << "Unable to open I2C file\n";
#endif	
	}

#if defined (ZPSIM)
	void EEPROMClass::saveEEPROM() {
		auto myfile = ofstream("EEPROM.dat", ios::binary);
		if (myfile) {
			myfile.write((char*)myEEProm, sizeof(myEEProm));
			cout << "EEPROM.dat Saved\n";
		}
		else cout << "Unable to open EEPRPM file";
		myfile = ofstream("I2C.dat", ios::binary);
		if (myfile) {
			myfile.write((char*)Wire1.i2CArr, sizeof(Wire1.i2CArr));
			cout << "I2C.dat Saved\n";
		}
		else cout << "Unable to open I2C file";
	}

	EEPROMClass::~EEPROMClass() {
#if defined LOAD_EEPROM
		saveEEPROM();
#endif
	}

	//EEPROMClass & EEPROM(fileEEPROM);
#endif

/******************************************************************************
 * User API
 ******************************************************************************/

//void EEPROMClass::setI2Chelper(I2C_Talk & i2C) {
//	_i2C = &i2C;
//	_i2C->result.reset();
//	_i2C->result.foundDeviceAddr = _eepromAddr;
//	_i2C->speedTestS();
//	_i2C->setThisI2CFrequency(_eepromAddr, 100000);
//}

uint8_t EEPROMClass::read(int iAddr)
{
	//#if defined (ZPSIM)
	//	return myEEProm[iAddr];
	//#else
		uint8_t result;
		//Serial.println("EP::Read");
		readEP(iAddr,1,&result);
		return result;
	//#endif
}

error_codes EEPROMClass::write(int iAddr, uint8_t iVal)
{
	//#if defined (ZPSIM)
	//	myEEProm[iAddr] = iVal;
	//	return 0;
	//#else
	  return writeEP(iAddr, iVal);
	//#endif
}

error_codes EEPROMClass::update(int iAddr, uint8_t iVal) {
	if (iVal != read(iAddr)) return write(iAddr,iVal);
	else return _OK; // ++iAddr;
}

#endif