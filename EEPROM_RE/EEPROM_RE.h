/*
  EEPROM.h - EEPROM library
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
#pragma once
//#pragma message( "I2C EEPROM header loaded" )

#if defined(__SAM3X8E__) || defined ZPSIM

#include "Arduino.h"
#include "I2C_Device.h"
#if defined ZPSIM
#define LOAD_EEPROM
#endif

#if defined (ZPSIM)
#include "..\HeatingSystem\src\HardwareInterfaces\A__Constants.h"
#endif

class I2C_Talk;

class EEPROMClassRE : public I_I2Cdevice_Recovery {
#if defined (ZPSIM)
public:
	//EEPROMClassRE();
	static uint8_t myEEProm[HardwareInterfaces::EEPROM_SIZE]; // EEPROM object may not get created until after it is used! So ensure array exists by making it static
	~EEPROMClassRE();
	void loadFile();
	void saveEEPROM();
#endif

public:
	// Rated at 100kHz/3v, 400kHz/5v, 0.7*Vdd HI = 3.5v
	EEPROMClassRE(int addr);
	EEPROMClassRE(I2C_Recovery::I2C_Recover& recover, int addr);
	uint8_t read(int iAddr);
	auto write(int iAddr, uint8_t iVal)->I2C_Talk_ErrorCodes::Error_codes;
	auto update(int iAddr, uint8_t iVal)->I2C_Talk_ErrorCodes::Error_codes;
	
	//Functionality to 'get' and 'put' objects to and from EEPROM.
	template< typename T > T & get(int iAddr, T & t) {
		uint8_t * ptr = (uint8_t*)&t;
		for (int endStop = iAddr + sizeof(T); iAddr < endStop; ++iAddr, ++ptr)  *ptr = read(iAddr);
		return t;
	}

	template< typename T > const T & put(int iAddr, const T & t) {
		const uint8_t * ptr = (const uint8_t*)&t;
		for (int endStop = iAddr + sizeof(T); iAddr < endStop; ++iAddr, ++ptr)  update(iAddr, *ptr);
		return t;
	}

	uint8_t readErr() { return _read_error; }
private:
	uint8_t _read_error = 0;
};

template<I2C_Talk & i2c>
class EEPROMClass_T : public EEPROMClassRE {
public:
	EEPROMClass_T(I2C_Recovery::I2C_Recover& recover, int addr) : EEPROMClassRE(recover, addr) {
		i2c.setMax_i2cFreq(100000);
		i2c.extendTimeouts(5000, 5, 1000);
	}	
	
	EEPROMClass_T(int addr) : EEPROMClassRE(addr) { 
		i2c.setMax_i2cFreq(100000);
		i2c.extendTimeouts(5000, 5, 1000);
	}
private:
	using EEPROMClassRE::i2C;
	 I2C_Talk & i2C() override { return i2c; }
};

EEPROMClassRE& eeprom();
using EEPROMClass = EEPROMClassRE;
#else
#include <EEPROM.h>
class EEPROMClassRE : public EEPROMClass{
public:
	using EEPROMClass::EEPROMClass;
	uint8_t readErr() { return 0; }
};
//#pragma message( "__SAM3X8E__ not defined" )
#endif
