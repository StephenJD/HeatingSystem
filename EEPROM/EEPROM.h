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

#include <inttypes.h>

#if defined(__SAM3X8E__)
class I2C_Helper;

class EEPROMClass
{
  public:
    EEPROMClass(I2C_Helper & i2C, uint8_t eepromAddr, uint8_t rtcAddr = 0);
    uint8_t read(int iAddr);
    uint8_t write(int iAddr, uint8_t iVal);
    uint8_t update(int iAddr, uint8_t iVal);	
 private:
   I2C_Helper & _i2C;
   uint8_t _eepromAddr;
   uint8_t _rtcAddr;
};

extern EEPROMClass & EEPROM;

#endif
