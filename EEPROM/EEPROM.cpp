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

#include "EEPROM.h"
#include "I2C_Helper.h"

/******************************************************************************
 * Constructors
 ******************************************************************************/
EEPROMClass::EEPROMClass(I2C_Helper & i2C, uint8_t eepromAddr, uint8_t rtcAddr) : _i2C(i2C), _eepromAddr(eepromAddr), _rtcAddr(rtcAddr) {
	//_i2C.result.foundDeviceAddr = _eepromAddr;
	//_i2C.speedTestNextS();
	//_i2C.result.foundDeviceAddr = _rtcAddr;
	//_i2C.speedTestNextS();
	//_i2C.setI2CFrequency(_i2C.result.maxSafeSpeed);
	_i2C.setI2CFrequency(100000);
}

/******************************************************************************
 * User API
 ******************************************************************************/

uint8_t EEPROMClass::read(int iAddr)
{
    uint8_t result;
	_i2C.readEP(_eepromAddr,iAddr,1,&result);
	return result;
}

uint8_t EEPROMClass::write(int iAddr, uint8_t iVal)
{
  uint8_t iRC = _i2C.writeEP(_eepromAddr,iAddr,iVal);
  delay(5);  // Give the EEPROM time to write its data  
  return(iRC);
}

uint8_t EEPROMClass::update(int iAddr, uint8_t iVal) {
	uint8_t iRC = 0;
	if (iVal != read(iAddr)) iRC = write(iAddr,iVal);
	return iRC;
}

EEPROMClass * EEPROM = 0;
