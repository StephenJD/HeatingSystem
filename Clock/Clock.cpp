#include "Clock.h"
#include <Timer_mS_uS.h>
#include <EEPROM.h>

using namespace I2C_Talk_ErrorCodes;
///////////////////////////////////////////////////
//         Public Global Functions        //
///////////////////////////////////////////////////

using namespace GP_LIB;
	using namespace Date_Time;

	uint8_t Clock::_mins1; // minute units
	uint8_t Clock::_secs;
	uint8_t Clock::_autoDST;
	uint8_t Clock::_dstHasBeenSet;
	Date_Time::DateTime Clock::_now;
	uint32_t Clock::_lastCheck_mS = millis() - uint32_t(60ul * 1000ul * 11ul);

	///////////////////////////////////////////////////
	//             Clock Member Functions            //
	///////////////////////////////////////////////////

	//Date_Time::DateTime Clock::_timeFromCompiler(int & minUnits, int & seconds) { // inlined to ensure latest compile time used.
	//// sample input: date = "Dec 26 2009", time = "12:34:56"
	//	auto year = GP_LIB::c2CharsToInt(&__DATE__[9]);
	//	// Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
	//	int mnth;
	//	switch (__DATE__[0]) {
	//	case 'J': mnth = __DATE__[1] == 'a' ? 1 : mnth = __DATE__[2] == 'n' ? 6 : 7; break;
	//	case 'F': mnth = 2; break;
	//	case 'A': mnth = __DATE__[2] == 'r' ? 4 : 8; break;
	//	case 'M': mnth = __DATE__[2] == 'r' ? 3 : 5; break;
	//	case 'S': mnth = 9; break;
	//	case 'O': mnth = 10; break;
	//	case 'N': mnth = 11; break;
	//	case 'D': mnth = 12; break;
	//	default: mnth = 0;
	//	}
	//	auto day = GP_LIB::c2CharsToInt(&__DATE__[4]);
	//	auto hrs = GP_LIB::c2CharsToInt(__TIME__);
	//	auto mins10 = GP_LIB::c2CharsToInt(&__TIME__[3]);
	//	minUnits = mins10 % 10;
	//	seconds = GP_LIB::c2CharsToInt(&__TIME__[6]);
	//	logger() << F("Compiler Time: ") << __DATE__ << " " << __TIME__ << L_endl;
	//	return { { day,mnth,year },{ hrs,mins10 } };
	//}

	DateTime Clock::_dateTime() const { 
		// called on every request for time/date.
		// Only needs to check anything every 10 minutes
		static uint8_t oldHr = _now.hrs() + 1; // force check first time through
		int newSecs = _secs + secondsSinceLastCheck(_lastCheck_mS);
		_secs = newSecs % 60;
		int newMins = _mins1 + newSecs / 60;
		_mins1 = newMins % 10;
		if (newMins >= 10) {
			Clock & modifierRef = const_cast<Clock &>(*this);
			_now.addOffset({ m10, newMins / 10 });
			modifierRef._update();
			if (_now.hrs() != oldHr) {
				oldHr = _now.hrs();
				modifierRef._adjustForDST();
			}
		}
		//Serial.print(" Refresh: "); Serial.println(_now.hrs());
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
	//                     Clock_EEPROM                          //
	///////////////////////////////////////////////////////////////

	Clock_EEPROM::Clock_EEPROM(unsigned int addr) :_addr(addr) { /*loadTime();*/ } // loadtime crashes!
	
	error_codes Clock_EEPROM::writer(uint16_t & address, const void * data, int noOfBytes) {
		const unsigned char * byteData = static_cast<const unsigned char *>(data);
		auto status = _OK;
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
#if defined(__SAM3X8E__)
			status |= eeprom().update(address, *byteData);
#else
			eeprom().update(address, *byteData);
#endif
		}
		return status;
	}

	error_codes Clock_EEPROM::reader(uint16_t & address, void * result, int noOfBytes) {
		uint8_t * byteData = static_cast<uint8_t *>(result);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
			*byteData = eeprom().read(address);
		}
		return _OK;
	}

	bool Clock_EEPROM::ok() const {
		int _day = day();
		return  _day > 0 && _day < 32;
	}

	error_codes Clock_EEPROM::saveTime() {
		auto nextAddr = _addr;
		auto status = writer(nextAddr, &_now, sizeof(_now));
		status |= writer(nextAddr, &_mins1, 1);
		status |= writer(nextAddr, &_secs, 1);
		status |= writer(nextAddr, &_autoDST, 1);
		status |= writer(nextAddr, &_dstHasBeenSet, 1);
		//logger() << F("Save EEPROM CurrDateTime...") << I2C_Talk::getStatusMsg(status) << L_endl;
		return status;
	}
	
	error_codes Clock_EEPROM::loadTime() {
		// retain and save _now unless it is invalid or before compiler-time
		auto eepromTime = _now;
		uint8_t eepromMins1;
		uint8_t eepromSecs;
		auto lastCompilerTime = _now;
		auto nextAddr = _addr;
		auto status = reader(nextAddr, &eepromTime, sizeof(_now));
		status |= reader(nextAddr, &eepromMins1, 1);
		status |= reader(nextAddr, &eepromSecs, 1);
		status |= reader(nextAddr, &_autoDST, 1);
		status |= reader(nextAddr, &_dstHasBeenSet, 1);
		reader(nextAddr, &lastCompilerTime, sizeof(_now));
		//Serial.print("Read lastCompilerTime: "); Serial.println(CStr_20(lastCompilerTime));
		if (!eepromTime.inRange() || !lastCompilerTime.inRange()) {
			eepromTime = DateTime{};
			lastCompilerTime = DateTime{};
		}
		//Serial.print("Now: "); Serial.print(CStr_20(_now)); Serial.print(" eepromTime: "); Serial.print(CStr_20(eepromTime)); Serial.print(" lastCompilerTime: "); Serial.println(CStr_20(lastCompilerTime));
		
		int compilerMins1, compilerSecs;
		auto compilerTime = _timeFromCompiler(compilerMins1, compilerSecs);

		if (compilerTime != lastCompilerTime) {
			_now = compilerTime;
			setMinUnits(compilerMins1);
			setSeconds(compilerSecs);
			saveTime();
			nextAddr = _addr + sizeof(_now) + 4;
			writer(nextAddr, &compilerTime, sizeof(_now));
			Serial.println(F("EE Clock Set from Compiler"));
		}
		else if (_now.inRange()) {
			saveTime();
			Serial.println(F("Clock saved to EEPROM"));
		}
		else {
			_now = eepromTime;
			setMinUnits(eepromMins1);
			setSeconds(eepromSecs);
			Serial.println(F("Clock Set from EEPROM"));
		}
		Serial.println();
		return status;
	}

	void Clock_EEPROM::_update() { // called every 10 minutes
		loadTime();
	}

	///////////////////////////////////////////////////////////////
	//                     Clock_I2C                             //
	///////////////////////////////////////////////////////////////
	
	DateTime I_Clock_I2C::_timeFromRTC(int & minUnits, int & seconds) {
		uint8_t data[7] = { 0 };
		// lambda
		auto dateInRange = [](const uint8_t * data) {
			//logger() << " Read: " << int(data[0]) << "s " << int(data[1]) << "m "
			//	<< int(data[2]) << "h " << int(data[3]) << "d "
			//	<< int(data[4]) << "d " << int(data[5]) << "mn "
			//	<< int(data[6]) << "y" << L_endl;
			if (data[0] > 89) return false; // BCD secs
			if (data[1] > 89) return false;
			if (data[2] > 35) return false;
			if (data[3] > 7) return false; // Day of week
			if (data[4] > 49) return false; // BCD Day of month
			if (data[5] > 18) return false; // BCD month
			if (data[6] > 114) return false; // BCD year
			return true;
		};

		auto timeout = Timer_mS(1000);
		auto status = _OK;
		do {
			status = readData(0, 7, data);
		} while (status != _OK && !timeout);
		//logger() << F("RTC read in ") << timeout.timeUsed() << I2C_Talk::getStatusMsg(status) << L_endl;
		
		DateTime date{};
		if (status == _OK && dateInRange(data)) {
			date.setMins10(data[1] >> 4);
			date.setHrs(fromBCD(data[2]));
			date.setDay(fromBCD(data[4]));
			date.setMonth(fromBCD(data[5]));
			if (date.month() == 0) date.setMonth(1);
			date.setYear(fromBCD(data[6]));
			minUnits = data[1] & 15;
			seconds = fromBCD(data[0]);
			logger() << F("RTC Time: ") << date << L_endl;
		}
		else {
			logger() << F("RTC Bad date.") << I2C_Talk::getStatusMsg(status) << L_endl;
		}
		return date;
	}

	error_codes I_Clock_I2C::loadTime() {	
		// lambda
		auto shouldUseCompilerTime = [](DateTime rtcTime, DateTime compilerTime) -> bool {
			if (rtcTime == DateTime{}) {
				if (millis() < 10000) return true; // don't want to reset RTC if this is a temporary glitch
			} else if (rtcTime < compilerTime) return true; // got valid rtc time
			return false;
		};

		int rtcMinUnits, rtcSeconds;
		auto rtcTime = _timeFromRTC(rtcMinUnits, rtcSeconds); // 0 if failed
		int compilerMinUnits, compilerSeconds;
		auto compilerTime = _timeFromCompiler(compilerMinUnits, compilerSeconds);
		auto status = _OK;
		_lastCheck_mS = millis();
		if (shouldUseCompilerTime(rtcTime,compilerTime)) {
			_now = compilerTime;
			setMinUnits(compilerMinUnits);
			setSeconds(compilerSeconds);
			saveTime();
			logger() << L_time << F(" RTC Clock Set from Compiler") << L_endl;
		}
		else {
			uint8_t dst;
			status = readData(8, 1, &dst);
			_now = rtcTime;
			setMinUnits(rtcMinUnits);
			setSeconds(rtcSeconds);
			_autoDST = dst >> 1;
			_dstHasBeenSet = dst & 1;
			logger() << L_time << F(" Clock Set from RTC") << L_endl;
		}
		return status;
	}

	error_codes I_Clock_I2C::saveTime() {

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
		auto status = writeData(0, 9, data);
		logger() << F(" Save RTC CurrDateTime...") << I2C_Talk::getStatusMsg(status) << L_endl;
		
		return status;
	}