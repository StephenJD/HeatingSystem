#pragma once
#include <../DateTime/src/Date_Time.h>
//#include <Date_Time.h>
#include <I2C_Talk.h>
#include <I2C_Device.h>
//#include <Conversions.h>
#include <..\Conversions\Conversions.h>
#include <Arduino.h>
#include <I2C_Talk_ErrorCodes.h>

//#pragma message( "Clock.h loaded" )

	/// <summary>
	/// A Monostate class representing current time/date.
	/// Date/Time obtained by explicit outward conversion to a Date_Time object.
	/// Derivations provide EEPROM storage of current time every 10 minutes,
	/// or obtaining time from an I2C RTC chip.
	/// Automatic DST may be set from 0-2 hours.
	/// </summary>

	class Clock { 
	public:
		// Rated at 5v/100kHz - NO 3.3v operation, min 2.2v HI.
		//Clock(const char* date, const char* time);
		// Queries
		int minUnits() const { return _mins1; }
		int seconds() const { return _secs; }
		int autoDSThours() const { return _autoDST; }
		int mins10() const { return _now.mins10(); }
		int hrs() const { return _now.hrs(); }
		int day() const { return _now.day(); }
		int month() const { return _now.month(); }
		int year() const { return _now.year(); }
		virtual bool ok() const { return true; }
	
		// Modifiers
		// Reading the time triggers an update-check which might modify the time
		explicit operator Date_Time::DateTime() { return _dateTime(); }
		Date_Time::DateTime now() { return  _dateTime(); }
		Date_Time::DateOnly date() { return _dateTime(); }
		Date_Time::TimeOnly time() { return _dateTime(); }
		uint32_t asInt() { return _dateTime().asInt(); }
		void refresh() { _dateTime(); }

		void setTime(Date_Time::DateOnly date, Date_Time::TimeOnly time, int min);
		void setTime(Date_Time::TimeOnly time) { _now.time() = time; saveTime();}
		void setDate(Date_Time::DateOnly date) { _now.date() = date; saveTime();}
		void setHrMin(int hr, int min);
		void setMinUnits(uint8_t minUnits) { _mins1 = minUnits; }
		void setSeconds(uint8_t secs) { _secs = secs; }
		void setAutoDSThours(int set) { _autoDST = set; saveTime(); }
		virtual auto saveTime()->I2C_Talk_ErrorCodes::error_codes { return I2C_Talk_ErrorCodes::_OK; }
		virtual auto loadTime()->I2C_Talk_ErrorCodes::error_codes { return I2C_Talk_ErrorCodes::_OK; }
		// Public Helper Functions
		static int secondsSinceLastCheck(uint32_t & lastCheck_mS);
		static uint32_t millisSince(uint32_t lastCheck_mS) { return millis() - lastCheck_mS; } // Since unsigned ints are used, rollover just works.

#ifdef ZPSIM
		void testAdd1Min();
#endif

	protected:
		virtual void _update() {} // called by _dateTime() every 10 minutes
		bool dstHasBeenSet() { return _dstHasBeenSet == 1; }
		void dstHasBeenSet(bool set) { _dstHasBeenSet = set; }
		void _setFromCompiler();

		static uint8_t _secs;
		static uint8_t _mins1; // minute units
		static uint8_t _autoDST; // hours to add/remove
		static uint8_t _dstHasBeenSet;
		static Date_Time::DateTime _now;
		static uint32_t _lastCheck_mS;

	private:
		Date_Time::DateTime _dateTime();
		void _adjustForDST();
	};

	class Clock_EEPROM : public Clock {
	public:
		Clock_EEPROM(unsigned int addr);
		auto saveTime()->I2C_Talk_ErrorCodes::error_codes override;
		bool ok() const override;
	private:
		void _update() override;  // called every 10 minutes - saves to EEPROM
		auto loadTime()->I2C_Talk_ErrorCodes::error_codes override;
		uint16_t _addr;
	};
	
	class I_Clock_I2C : public Clock {
	public:
		auto saveTime()->I2C_Talk_ErrorCodes::error_codes override;
	protected:
		auto loadTime()->I2C_Talk_ErrorCodes::error_codes override;

	private:
		void _update() override; // called every 10 minutes - reads from RTC
		virtual auto readData(uint16_t start, uint16_t numberBytes, uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes = 0;
		virtual auto writeData(uint16_t start, uint16_t numberBytes, uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes = 0;
	};

	template<I2C_Talk & i2c>
	class Clock_I2C : public I_Clock_I2C, public I2Cdevice<i2c> {
	public:
		using I2Cdevice<i2c>::I2Cdevice;

		Clock_I2C(int addr) : I2Cdevice<i2c>(addr) {
			loadTime();
		}
		
		bool ok() const override {
			for (uint32_t t = millis() + 5; millis() < t; ) {
				if (i2c.status(I2Cdevice<i2c>::getAddress()) == I2C_Talk_ErrorCodes::_OK) return true;
			}
			return false;
		}
		I2C_Talk_ErrorCodes::error_codes testDevice() override;

	private:
		auto readData(uint16_t start, uint16_t numberBytes, uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes override { return i2c.read(I2Cdevice<i2c>::getAddress(), start, numberBytes, dataBuffer); }
		auto writeData(uint16_t start, uint16_t numberBytes, uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes override { return i2c.write(I2Cdevice<i2c>::getAddress(), start, numberBytes, dataBuffer); }
	};


///////////////////////////////////////////////////////////////
//                         Clock                             //
///////////////////////////////////////////////////////////////

	Clock & clock_();  // to be defined by the client

	inline void Clock::_setFromCompiler() { // inlined to ensure latest compile time used.
		// sample input: date = "Dec 26 2009", time = "12:34:56"
		auto year = GP_LIB::c2CharsToInt(&__DATE__[9]);
		// Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
		int mnth;
		switch (__DATE__[0]) {
		case 'J': mnth = __DATE__[1] == 'a' ? 1 : mnth = __DATE__[2] == 'n' ? 6 : 7; break;
		case 'F': mnth = 2; break;
		case 'A': mnth = __DATE__[2] == 'r' ? 4 : 8; break;
		case 'M': mnth = __DATE__[2] == 'r' ? 3 : 5; break;
		case 'S': mnth = 9; break;
		case 'O': mnth = 10; break;
		case 'N': mnth = 11; break;
		case 'D': mnth = 12; break;
		default: mnth = 0;
		}
		auto day = GP_LIB::c2CharsToInt(&__DATE__[4]);
		auto hrs = GP_LIB::c2CharsToInt(__TIME__);
		auto mins10 = GP_LIB::c2CharsToInt(&__TIME__[3]);
		_now = Date_Time::DateTime{ { day,mnth,year },{ hrs,mins10 } };
		setMinUnits(mins10 % 10);
		setSeconds(GP_LIB::c2CharsToInt(&__TIME__[6]));
		saveTime();
	}

///////////////////////////////////////////////////////////////
//                     Clock_I2C                             //
///////////////////////////////////////////////////////////////
//I2C_Talk * Clock_I2C::_i2C;

//Clock_I2C::Clock_I2C(I2C_Talk & i2C, int addr) : Clock_I2C(i2C, addr) {

	template<I2C_Talk & i2C>
	I2C_Talk_ErrorCodes::error_codes Clock_I2C<i2C>::testDevice() {
		//Serial.print(" RTC testDevice at "); Serial.println(i2c.getI2CFrequency(),DEC);
		uint8_t data[1] = { 0 };
		auto errCode = I2Cdevice<i2C>::write_verify(9, 1, data);
		data[0] = 255;
		if (errCode != I2C_Talk_ErrorCodes::_OK) errCode = I2Cdevice<i2C>::write_verify(9, 1, data);
		return errCode;
	}


