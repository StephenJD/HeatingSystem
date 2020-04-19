#include "Clock.h"
#include <EEPROM.h>

using namespace I2C_Talk_ErrorCodes;

	error_codes writer(uint16_t & address, const void * data, int noOfBytes) {
		const unsigned char * byteData = static_cast<const unsigned char *>(data);
		auto status = _OK;
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
		#if defined(__SAM3X8E__)
			status |= EEPROM.update(address, *byteData);
		#else
			EEPROM.update(address, *byteData);
		#endif
		}
		return status;
	}

	error_codes reader(uint16_t & address, void * result, int noOfBytes) {
		uint8_t * byteData = static_cast<uint8_t *>(result);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
			*byteData = EEPROM.read(address);
		}
		return _OK;
	}

	using namespace GP_LIB;
	using namespace Date_Time;

	uint8_t Clock::_mins1; // minute units
	uint8_t Clock::_secs;
	uint8_t Clock::_autoDST;
	uint8_t Clock::_dstHasBeenSet;
	Date_Time::DateTime Clock::_now;
	uint32_t Clock::_lastCheck_mS = millis() - uint32_t(60ul * 1000ul * 11ul);

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

	bool Clock_EEPROM::ok() const {
		int _day = day();
		return  _day > 0 && _day < 32;
	}

	error_codes Clock_EEPROM::saveTime() {
		auto nextAddr = _addr;
		auto status = writer(nextAddr, &_now, 4);
		status |= writer(nextAddr, &_autoDST, 1);
		status |= writer(nextAddr, &_dstHasBeenSet, 1);
		logger() << L_time << "Save EEPROM CurrDateTime..." << I2C_Talk::getStatusMsg(status) << L_endl;
		return status;
	}
	
	error_codes Clock_EEPROM::loadTime() {
		auto nextAddr = _addr;
		auto status = reader(nextAddr, &_now, 4);
		status |= reader(nextAddr, &_autoDST, 1);
		status |= reader(nextAddr, &_dstHasBeenSet, 1);
		int minUnits, seconds;
		auto compilerTime = _timeFromCompiler(minUnits, seconds);
		if (compilerTime > _now) {
			_now = compilerTime;
			setMinUnits(minUnits);
			setSeconds(seconds);
			saveTime();
			logger() << L_time << "Clock Set from Compiler" << L_endl;
		} else logger() << L_time << "Clock Set from EEPROM" << L_endl;


		return status;
	}

	void Clock_EEPROM::_update() { // called every 10 minutes
		saveTime();
	}

	///////////////////////////////////////////////////////////////
	//                     Clock_I2C                             //
	///////////////////////////////////////////////////////////////
	
	DateTime I_Clock_I2C::_timeFromRTC(int & minUnits, int & seconds) {
		uint8_t data[7] = { 0 };
		auto status = readData(0, 7, data);
		
		DateTime date{};
		if (status != _OK) {
			logger() << L_time << "RTC Unreadable." << I2C_Talk::getStatusMsg(status) << L_endl;
		}
		else {
			date.setMins10(data[1] >> 4);
			date.setHrs(fromBCD(data[2]));
			date.setDay(fromBCD(data[4]));
			date.setMonth(fromBCD(data[5]));
			if (date.month() == 0) date.setMonth(1);
			date.setYear(fromBCD(data[6]));
			minUnits = data[1] & 15;
			seconds = fromBCD(data[0]);
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

		if (shouldUseCompilerTime(rtcTime,compilerTime)) {
			_now = compilerTime;
			setMinUnits(compilerMinUnits);
			setSeconds(compilerSeconds);
			saveTime();
			logger() << L_time << "Clock Set from Compiler" << L_endl;
		}
		else {
			uint8_t dst;
			status = readData(8, 1, &dst);
			if (rtcTime != DateTime{} || status != _OK) {
				logger() << L_time << "RTC Unreadable." << I2C_Talk::getStatusMsg(status) << L_endl;
			}
			else {
				_now = rtcTime;
				setMinUnits(rtcMinUnits);
				setSeconds(rtcSeconds);
				_autoDST = dst >> 1;
				_dstHasBeenSet = dst & 1;
				logger() << L_time << "Clock Set from RTC" << L_endl;
			}
		}
	
		_lastCheck_mS = millis();
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

		logger() << L_time << "Save RTC CurrDateTime..." << I2C_Talk::getStatusMsg(status) << L_endl;
		return status;
	}