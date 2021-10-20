#pragma once
#include <Arduino.h>
#include <I2C_Talk.h>

#if defined(__SAM3X8E__)
#else
	#ifndef ZPSIM
	#include <MsTimer2.h>

	inline void stopTimer() { MsTimer2::stop(); digitalWrite(LED_BUILTIN, LOW); }
	inline void flashLED(int period) {
		pinMode(LED_BUILTIN, OUTPUT);
		digitalWrite(LED_BUILTIN, HIGH);
		MsTimer2::set(period, stopTimer);
		MsTimer2::start();
	}
	#endif
#endif

namespace i2c_registers {
	struct Defaut_Tag_None {};

	class I_Registers {
	public:
		uint8_t getRegister(int reg) const { return const_cast<I_Registers*>(this)->regArr()[reg]; }
		void setRegister(int reg, uint8_t value) { regArr()[reg] = value; }
		void addToRegister(int reg, uint8_t increment) { regArr()[reg] += increment; }
		uint8_t* reg_ptr(int reg) { return regArr() + reg; }
	protected:
		virtual uint8_t* regArr() = 0;
		virtual uint8_t& regAddr() = 0;
	};

	// mono-state class - all static data. Each template instantiation gets its own data.
#if defined(ZPSIM)
	template<int register_size, int i2C_addr>
#else
	template<int register_size, typename PurposeTag = Defaut_Tag_None>
#endif
	class Registers : public I_Registers {
	public:
		Registers(I2C_Talk& i2C) { _i2C = &i2C; }
		int noOfRegisters() { return register_size; }

		// Called when data is sent by a Master, telling this slave how many bytes have been sent.
		static void receiveI2C(int howMany) {
			_i2C->receiveFromMaster(1, &_regAddr); // first byte is reg-address
			if (--howMany) {
#if !defined(ZPSIM) && !defined(__SAM3X8E__)
				//flashLED(10);
#endif
				if (_regAddr + howMany > register_size) howMany = register_size - _regAddr;
				auto noReceived = _i2C->receiveFromMaster(howMany, _regArr + _regAddr);
				//Serial.flush(); Serial.print(noReceived); Serial.print(F(" for RAdr: ")); Serial.print((int)_regAddr); Serial.print(F(" V: ")); Serial.println((int)_regArr[_regAddr]);
			}
		}

		// Called when data is requested by a Master from this slave, reading from the last register address sent.
		static void requestI2C() { 
			// The Master will send a NACK when it has received the requested number of bytes, so we should offer as many as could be requested.
#if !defined(ZPSIM) && !defined(__SAM3X8E__)
			//flashLED(50);
#endif
			int bytesAvaiable = register_size - _regAddr;
			if (bytesAvaiable > 32) bytesAvaiable = 32;
			_i2C->write(_regArr + _regAddr, bytesAvaiable);
			//Serial.flush(); Serial.print(F("SAdr:")); Serial.print((int)_regAddr); 
			//Serial.print(F(" Len:")); Serial.print((int)bytesAvaiable);
			//Serial.print(F(" Val:")); Serial.println((int)*(_regArr + _regAddr));
		}
	private:
		uint8_t* regArr() override {return _regArr;}
		uint8_t& regAddr() override {return _regAddr;}
		static I2C_Talk* _i2C;
#ifdef ZPSIM
		static uint8_t* _regArr;
#else
		static uint8_t _regArr[register_size];
#endif
		static uint8_t _regAddr; // the register address sent in the request
	};


#ifdef ZPSIM
#include "Wire.h"
	template<int register_size, int i2C_addr>
	I2C_Talk* Registers<register_size, i2C_addr>::_i2C;

	template<int register_size, int i2C_addr>
	uint8_t* Registers<register_size, i2C_addr>::_regArr = (uint8_t*)&Wire.i2CArr[i2C_addr];

	template<int register_size, int i2C_addr>
	uint8_t Registers<register_size, i2C_addr>::_regAddr;
#else
	template<int register_size, typename PurposeTag>
	I2C_Talk* Registers<register_size, PurposeTag>::_i2C;

	template<int register_size, typename PurposeTag>
	uint8_t Registers<register_size, PurposeTag>::_regArr[register_size];

	template<int register_size, typename PurposeTag>
	uint8_t Registers<register_size, PurposeTag>::_regAddr;
#endif
}