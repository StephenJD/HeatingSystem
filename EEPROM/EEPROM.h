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

#if defined(__SAM3X8E__) || defined ZPSIM

#include "Arduino.h"
#define LOAD_EEPROM

class I2C_Helper;

class EEPROMClass
{
#if defined (ZPSIM)
public:
	static uint8_t myEEProm[4096]; // EEPROM object may not get created until after it is used! So ensure array exists by making it static
	EEPROMClass() {}
	~EEPROMClass();
#endif

  public:
    EEPROMClass(I2C_Helper * i2C, uint8_t eepromAddr);
    uint8_t read(int iAddr);
    uint8_t write(int iAddr, uint8_t iVal);
    uint8_t update(int iAddr, uint8_t iVal);
	void setI2Chelper(I2C_Helper & i2C);
 private:
   I2C_Helper * _i2C = 0;
   uint8_t _eepromAddr;
};

extern EEPROMClass & EEPROM;

#endif
