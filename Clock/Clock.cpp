#include <EEPROM.h>
#include "Clock.h"
#include <I2C_Talk_ErrorCodes.h>

using namespace I2C_Talk_ErrorCodes;

	int writer(int address, const void * data, int noOfBytes) {
		const unsigned char * byteData = static_cast<const unsigned char *>(data);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
			EEPROM.update(address, *byteData);
		}
		return address;
	}

	int reader(int address, void * result, int noOfBytes) {
		uint8_t * byteData = static_cast<uint8_t *>(result);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
			*byteData = EEPROM.read(address);
		}
		return address;
	}

	using namespace GP_LIB;
	using namespace Date_Time;

	uint8_t Clock::_mins1; // minute units
	uint8_t Clock::_secs;
	uint8_t Clock::_autoDST;
	uint8_t Clock::_dstHasBeenSet;
	Date_Time::DateTime Clock::_now;
	uint32_t Clock::_lastCheck_mS = millis();

	///////////////////////////////////////////////////
	//         Public Static Helper Functions        //
	///////////////////////////////////////////////////

	int Clock::secondsSinceLastCheck(uint32_t & lastCheck_mS) { // static function
		// move forward one second every 1000 milliseconds
		uint32_t elapsedTime = millisSince(lastCheck_mS);
		int seconds = 0;
		while (elapsedTime >= 1000) {
			lastCheck_mS += 1000;
			elapsedTime -= 1000;
			++seconds;
		}
		return seconds;
	}

	///////////////////////////////////////////////////
	//             Clock Member Functions            //
	///////////////////////////////////////////////////

	DateTime Clock::_dateTime() { 
		// called on every request for time/date.
		// Only needs to check anything every 10 minutes
		static uint8_t oldHr = _now.hrs() + 1; // force check first time through
		int newSecs = _secs + secondsSinceLastCheck(_lastCheck_mS);
		int newMins = _mins1 + newSecs / 60;
		_mins1 = newMins % 10;
		_secs = newSecs % 60;
		if (newMins < 10) return _now;

		_now.addOffset({ m10, newMins / 10 });
		_update();
		if (_now.hrs() != oldHr) {
			oldHr = _now.hrs();
			_adjustForDST();
		}
		return _now;
	}

	void Clock::setHrMin(int hr, int min) {
		_now.time() = TimeOnly(hr, min);
		setMinUnits(min % 10);
		saveTime();
	}

	void Clock::setTime(Date_Time::DateOnly date, Date_Time::TimeOnly time, int min) {
		_now = DateTime{ date, time };
		setMinUnits(min);
		setAutoDSThours(true);
		setSeconds(0);
		saveTime();
	}

	void Clock::_adjustForDST() {
		//Last Sunday in March and October. Go forward at 1 am, back at 2 am.
		if ((autoDSThours()) && (_now.month() == 3 || _now.month() == 10)) { // Mar/Oct
			if (_now.day() == 1 && dstHasBeenSet()) {
				dstHasBeenSet(false); // clear
				saveTime();
			}
			else if (_now.day() > 24) { // last week
				if (_now.weekDayNo() == SUNDAY) {
					if (!dstHasBeenSet() && _now.month() == 3 && _now.hrs() == 1) {
						_now.setHrs(autoDSThours()+1);
						dstHasBeenSet(true); // set
						saveTime();
					}
					else if (!dstHasBeenSet() && _now.month() == 10 && _now.hrs() == 2) {
						_now.setHrs(2-autoDSThours());
						dstHasBeenSet(true); // set
						saveTime();
					}
				}
			}
		}
	}

#ifdef ZPSIM
	void Clock::testAdd1Min() {
		if (minUnits() < 9) setMinUnits(minUnits() + 1);
		else {
			setMinUnits(0);
			_now.addOffset({ m10,1 });
		}
		_update();
	}
#endif
	
	///////////////////////////////////////////////////////////////
	//                     EEPROM_Clock                          //
	///////////////////////////////////////////////////////////////

	EEPROM_Clock::EEPROM_Clock(unsigned int addr) :_addr(addr) { loadTime(); }

	void EEPROM_Clock::saveTime() {
		auto nextAddr = writer(_addr, &_now, 4);
		nextAddr = writer(nextAddr, &_autoDST, 1);
		writer(nextAddr, &_dstHasBeenSet, 1);
	}
	
	void EEPROM_Clock::loadTime() {
		auto nextAddr = reader(_addr, &_now, 4);
		nextAddr = reader(nextAddr, &_autoDST, 1);
		nextAddr = reader(nextAddr, &_dstHasBeenSet, 1);
		if (year() == 0) {
			_setFromCompiler();
		}
	}

	void EEPROM_Clock::_update() { // called every 10 minutes
		saveTime();
	}


	///////////////////////////////////////////////////////////////
	//                     I2C_Clock                             //
	///////////////////////////////////////////////////////////////

	I2C_Clock::I2C_Clock(I2C_Talk & i2C, int addr) : I_I2Cdevice(i2C, addr) {
		i2C_speedTest();
		loadTime(); 
	}
	
	uint8_t I2C_Clock::i2C_speedTest() {
		//i2c_Talk().result.reset();
		//i2c_Talk().result.foundDeviceAddr = getAddress();
		//i2c_Talk().speedTestS(this);
		//i2c_Talk().setThisI2CFrequency(getAddress(), 100000);
		//loadTime(); 
		//return i2c_Talk().result.error;
		return 0;
	}

	uint8_t I2C_Clock::testDevice() {
		//Serial.print(" RTC testDevice at "); Serial.println(i2c.getI2CFrequency(),DEC);
		uint8_t data[1] = { 0 };
		auto errCode = write_verify(9, 1, data);
		data[0] = 255;
		if (errCode != _OK) errCode = write_verify(9, 1, data);
		return errCode;
	}

	void I2C_Clock::loadTime() {
		//Serial.println("I2C_Clock::loadTime()");
		// called by log so must not recursivly call log
		int errCode = 0;
#if !defined (ZPSIM)
		//if (_i2C) {
			uint8_t data[9];
			data[6] = 0; // year

			errCode = i2c_Talk().read(getAddress(), 0, 9, data);
			if ((errCode != _OK || data[6] == 255) && i2c_Talk().getI2CFrequency() > i2c_Talk().MIN_I2C_FREQ) {
				if (Serial) {
					Serial.print("Error reading RTC : "); Serial.print(i2c_Talk().getStatusMsg(errCode));
					Serial.print(" Year : "); Serial.println((int)data[6]);
				}
				if (Serial) { for (int val : data) { Serial.print(" data[] : "); Serial.println(fromBCD(val)); } }
				//i2c_Talk().slowdown_and_reset(0);
				data[6] = 0;
				errCode = i2c_Talk().read(getAddress(), 0, 9, data);
			}
			if (errCode != _OK) {
				if (Serial) { Serial.print("RTC Unreadable. "); Serial.println(i2c_Talk().getStatusMsg(errCode)); }
			}
			else if (data[6] == 0) {
				if (Serial) { Serial.println("RTC set from Compiler"); }
				_setFromCompiler();
			}
			else {
				//if (Serial) Serial.println("Set from RTC");
				_now.setMins10(data[1] >> 4);
				_now.setHrs(fromBCD(data[2]));
				_now.setDay(fromBCD(data[4]));
				_now.setMonth(fromBCD(data[5]));
				_now.setYear(fromBCD(data[6]));
				_autoDST = data[8] >> 1;
				_dstHasBeenSet = data[8] & 1;
				setMinUnits(data[1] & 15);
				setSeconds(fromBCD(data[0]));
			}
		//}
		//else {
		//	if (Serial) { Serial.println("No i2c: Set from Compiler"); }
		//	_setFromCompiler();
		//	errCode = _I2C_Device_Not_Found;
		//}
#endif	
	}

	void I2C_Clock::saveTime() {
		//Serial.print("Save CurrDateTime... at "); Serial.println(i2c_Talk().getThisI2CFrequency(0x68), DEC);

		uint8_t data[9];
		data[0] = toBCD(_secs); // seconds
		data[1] = (_now.mins10() << 4) + minUnits();
		data[2] = toBCD(_now.hrs());
		data[3] = 0; // day of week
		data[4] = toBCD(_now.day());
		data[5] = toBCD(_now.month());
		data[6] = toBCD(_now.year());
		data[7] = 0; // disable SQW
		data[8] = _autoDST << 1 | _dstHasBeenSet; // in RAM

		auto errCode = write(0, 9, data);

		//if (errCode != _OK && i2c_Talk().getI2CFrequency() > i2c_Talk().MIN_I2C_FREQ) {
		//	if (Serial) { Serial.print("Error writing RTC : ");  Serial.println(i2c_Talk().getStatusMsg(errCode)); }
		//	//i2c_Talk().slowdown_and_reset(0);
		//	errCode = write(0, 9, data);
		//}
		//if (errCode != _OK) {
		//	if (Serial) { Serial.print("Unable to write RTC:");  Serial.println(i2c_Talk().getStatusMsg(errCode)); }
		//}
		//else {
		//	//if (Serial) Serial.println("Saved CurrDateTime");
		//}
		//#if defined ZPSIM
		//	CURR_TIME = currentTime();
		//#endif
	}

	void I2C_Clock::_update() { // called every 10 minutes
		loadTime();
	}
