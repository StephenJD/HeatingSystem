#pragma once
#include <../DateTime/src/Date_Time.h>
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <..\Conversions\Conversions.h>
#include <Arduino.h>
#include <I2C_Talk_ErrorCodes.h>
#include <FlashStrings.h>

//#pragma message( "Clock.h loaded" )

	/// <summary>
	/// A Monostate class representing current time/date.
	/// NOTE: to get minute units and seconds, specific functions must be called.
	/// Date/Time obtained by explicit outward conversion to a Date_Time object.
	/// Derivations update time from an I2C RTC chip every 10 minutes.
	/// Automatic DST may be set from 0-2 hours.
	/// </summary>
	class Clock { 
	public:
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
	
		// Conceptually, these are queries, although reading the time triggers an update-check which might modify the time
		explicit operator Date_Time::DateTime() const { return _dateTime(); }
		
		/// <summary>
		/// Returns time in 10's of minutes - no units.
		/// </summary>
		Date_Time::DateTime now() const { return  _dateTime(); }
		Date_Time::DateOnly date() const { return _dateTime(); }
		Date_Time::TimeOnly time() const { return _dateTime(); }
		uint32_t asInt() const { return _dateTime().asInt(); }

		// Modifiers
		void refresh() const { _dateTime(); }

		void setTime(Date_Time::DateOnly date, Date_Time::TimeOnly time, int min);
		void setTime(Date_Time::TimeOnly time) { _now.time() = time; saveTime();}
		void setDate(Date_Time::DateOnly date) { _now.date() = date; saveTime();}
		void setHrMin(int hr, int min);
		void setMinUnits(uint8_t minUnits) { _mins1 = minUnits; }
		void setSeconds(uint8_t secs) { _secs = secs; }
		void setAutoDSThours(int set) { _autoDST = set; saveTime(); }
		virtual auto saveTime()->I2C_Talk_ErrorCodes::error_codes { return I2C_Talk_ErrorCodes::_OK; }
		virtual auto loadTime()->I2C_Talk_ErrorCodes::error_codes { return I2C_Talk_ErrorCodes::_OK; }

#ifdef ZPSIM
		void testAdd1Min();
#endif

	protected:
		virtual void _update() {} // called by _dateTime() every 10 minutes
		bool dstHasBeenSet() const { return _dstHasBeenSet == 1; }
		void dstHasBeenSet(bool set) const { _dstHasBeenSet = set; }
		auto _timeFromCompiler(int & minUnits, int & seconds) -> Date_Time::DateTime;

		static uint8_t _secs;
		static uint8_t _mins1; // minute units
		static uint8_t _autoDST; // hours to add/remove
		static uint8_t _dstHasBeenSet;
		static Date_Time::DateTime _now;
		static uint32_t _lastCheck_mS;

	private:
		Date_Time::DateTime _dateTime() const;
		void _adjustForDST();
	};

	inline Logger & operator<<(Logger & logger, const Clock & clk) {
		clk.refresh();
		return logger << GP_LIB::intToString(clk.day(),2)
		<< F_SLASH << GP_LIB::intToString(clk.month(),2)
		<< F_SLASH << GP_LIB::intToString(clk.year(),2)
		<< F_SPACE << GP_LIB::intToString(clk.hrs(),2)
		<< F_COLON << clk.mins10() << clk.minUnits()
		<< F_COLON << GP_LIB::intToString(clk.seconds(),2);
	}

	/// <summary>
	/// A Monostate class representing current time/date.
	/// Last compile-time saved to EEPROM (4-bytes)
	/// Saves 6-byte time to EEPROM every 10 minutes to aid recovery from re-start.
	/// </summary>	
	class Clock_EEPROM : public Clock {
	public:
		Clock_EEPROM(unsigned int addr);
		static constexpr int SIZE = sizeof(_now) * 2 + 4;
		auto saveTime()->I2C_Talk_ErrorCodes::error_codes override;
		bool ok() const override;
	private:
		void _update() override;  // called every 10 minutes - saves to EEPROM
		auto loadTime()->I2C_Talk_ErrorCodes::error_codes override;
		auto writer(uint16_t & address, const void * data, int noOfBytes) -> I2C_Talk_ErrorCodes::error_codes;
		auto reader(uint16_t & address, void * result, int noOfBytes) -> I2C_Talk_ErrorCodes::error_codes;
		
		uint16_t _addr;
	};
	
	class I_Clock_I2C : public Clock {
	public:
		auto saveTime()->I2C_Talk_ErrorCodes::error_codes override;
	protected:
		auto loadTime()->I2C_Talk_ErrorCodes::error_codes override;

	private:
		void _update() override { loadTime(); }	// called every 10 minutes - reads from RTC
		virtual auto readData(int start, int numberBytes, uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes = 0;
		virtual auto writeData(int start, int numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes = 0;
		auto _timeFromRTC(int & minUnits, int & seconds) -> Date_Time::DateTime;
	};

	template<I2C_Talk & i2c>
	class Clock_I2C : public I_Clock_I2C, public I2Cdevice<i2c> {
	public:
		using I2Cdevice<i2c>::I2Cdevice;

		Clock_I2C(int addr) : I2Cdevice<i2c>(addr) {
			// Rated at 5v/100kHz - NO 3.3v operation, min 2.2v HI.
			i2c.setMax_i2cFreq(100000); // Max freq supported by DS1307
			i2c.extendTimeouts(5000, 5, 1000); // process time to read clock. Potential freeze if Wire not constructed
			//Serial.println(F("Clock_I2C Constructor"));
		}
		
		bool ok() const override {
			for (uint32_t t = millis() + 5; millis() < t; ) {
				if (i2c.status(I2Cdevice<i2c>::getAddress()) == I2C_Talk_ErrorCodes::_OK) return true;
			}
			return false;
		}
		auto testDevice() -> I2C_Talk_ErrorCodes::error_codes override;

	private:
		auto readData(int start, int numberBytes, uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes override {
			//if (I2Cdevice<i2c>::getStatus() != I2C_Talk_ErrorCodes::_OK) logger() << "RTC Unavailable" << L_endl;
			return I2Cdevice<i2c>::read(start, numberBytes, dataBuffer); 
		}
		auto writeData(int start, int numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes override { return I2Cdevice<i2c>::write_verify(start, numberBytes, dataBuffer); }
	};

///////////////////////////////////////////////////////////////
//                         Clock                             //
///////////////////////////////////////////////////////////////

	Clock & clock_();  // to be defined by the client

	inline Date_Time::DateTime Clock::_timeFromCompiler(int & minUnits, int & seconds) { // inlined to ensure latest compile time used.
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
		minUnits = mins10 % 10;
		seconds = GP_LIB::c2CharsToInt(&__TIME__[6]);
		Serial.print(F("Compiler Time: ")); Serial.print(__DATE__); Serial.print(F(" ")); Serial.println(__TIME__);
		return { { day,mnth,year },{ hrs,mins10 } };
	}

///////////////////////////////////////////////////////////////
//                     Clock_I2C                             //
///////////////////////////////////////////////////////////////
//I2C_Talk * Clock_I2C::_i2C;

//Clock_I2C::Clock_I2C(I2C_Talk & i2C, int addr) : Clock_I2C(i2C, addr) {

	template<I2C_Talk & i2C>
	auto Clock_I2C<i2C>::testDevice() -> I2C_Talk_ErrorCodes::error_codes {
		//Serial.print(" RTC testDevice at "); Serial.println(i2c.getI2CFrequency(),DEC);
		uint8_t data[1] = { 0 };
		auto errCode = I2Cdevice<i2C>::write_verify(9, 1, data);
		data[0] = 255;
		if (errCode != I2C_Talk_ErrorCodes::_OK) errCode = I2Cdevice<i2C>::write_verify(9, 1, data);
		return errCode;
	}


