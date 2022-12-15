#pragma once
#include <Arduino.h>
#include <I2C_Talk.h>

namespace i2c_registers {

	struct Defaut_Tag_None {};
	/// <summary>
	/// Registers are in static-storage and thus initialised to zero
	/// No public access. Must use RegAccess.
	/// </summary>
	class I_Registers {
	private:
		friend class RegAccess;
		uint8_t getRegister(int reg) const { return const_cast<I_Registers*>(this)->regArr()[validReg(reg)]; }
		virtual int noOfRegisters() const = 0;
		void setRegister(int reg, uint8_t value) { regArr()[validReg(reg)] = value; }
		bool updateRegister(int reg, uint8_t value) {
			reg = validReg(reg);
			bool hasChanged = regArr()[reg] != value;
			if (hasChanged) regArr()[reg] = value;
			return hasChanged;
		}
		void addToRegister(int reg, uint8_t increment) { regArr()[validReg(reg)] += increment; }
		volatile uint8_t* reg_ptr(int reg) { return regArr() + validReg(reg); }
		virtual volatile uint8_t* regArr() = 0;
		virtual uint8_t& regAddr() = 0;
		uint8_t validReg(int reg) const {
			bool inRange = true;
			if (reg >= noOfRegisters()) {
				reg = noOfRegisters() - 1;
				inRange = false;
			}
			if (reg < 0) {
				reg = 0;
				inRange = false;
			}
			if (!inRange) logger() << L_time << F("Reg out of range: ") << reg << L_flush;
			return reg;
		}
	protected:
		static I2C_Talk* _i2C;
	};

	/// <summary>
	/// Provides public access to registers.
	/// Constructed with refernce to registers and register-offset.
	/// It allows multiple remote registers to map to a single local register
	/// applying any offset required.
	/// RemoteReg : ...........{I,J,K}
	/// LocalReg:   {{A,B,C,D},{I,J,K},{V,W,X,Y,Z}} Offset by 4 required.
	/// </summary>
	class RegAccess {
	public:
		RegAccess(I_Registers& registers, int regOffset = 0);
		uint8_t get(int reg) const { return _registers.getRegister(_regOffset + reg); }
		uint8_t validReg(int reg) const { return _registers.validReg(reg); }
		void set(int reg, uint8_t value) { return _registers.setRegister(_regOffset + reg, value); }
		bool update(int reg, uint8_t value) { return _registers.updateRegister(_regOffset + reg, value); }
		void add(int reg, uint8_t increment) { _registers.addToRegister(_regOffset + reg, increment); }
		volatile uint8_t* const ptr(int reg) /* can't move pointer */ { return _registers.reg_ptr(_regOffset + reg); }
	private:
		I_Registers& _registers;
		uint8_t _regOffset = 0;
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
		int noOfRegisters() const override { return register_size; }

		// Called when data is sent by a Master, telling this slave how many bytes have been sent.
		// receiveI2C could start during a RegisterUpdate/Read, but RegisterUpdate/Read cannot start during receiveI2C.
		// The bytes have already been sent when this gets called.
		// Slave can hold CLK low until it is ready to receive data.
		static void receiveI2C(int howMany) {
			uint8_t regNo;
			if (_i2C->receiveFromMaster(1, &regNo) > 0) {
				_regNumber = regNo; // first byte is reg-number
				if (_regNumber >= register_size) _regNumber = register_size - 1;
			}
			if (--howMany) {
				if (_regNumber + howMany > register_size) howMany = register_size - _regNumber;
				_i2C->receiveFromMaster(howMany, _regArr + _regNumber);
			}
		}

		// Called when data is requested by a Master from this slave, reading from the last register address sent.
		// requestI2C could start during a RegisterUpdate, but RegisterUpdate cannot start during requestI2C.
		// Writes bytes to the transmit buffer. When it returns the bytes are sent.
		static void requestI2C() {
			// The Master will send a NACK when it has received the requested number of bytes, so we should offer as many as could be requested.
			int bytesAvaiable = register_size - _regNumber;
			if (bytesAvaiable > 32) bytesAvaiable = 32;
			_i2C->write(_regArr + _regNumber, bytesAvaiable);
		}
	private:
		volatile uint8_t* regArr() override { return _regArr; }
		uint8_t& regAddr() override { return _regNumber; }
#ifdef ZPSIM
		volatile static uint8_t* _regArr;
#else
		volatile static uint8_t _regArr[register_size];
#endif
		static uint8_t _regNumber; // the register number sent in the request
	};

	// Static Definitions
#ifdef ZPSIM
#include "Wire.h"
	template<int register_size, int i2C_addr>
	volatile uint8_t* Registers<register_size, i2C_addr>::_regArr = &TwoWire::i2CArr[i2C_addr][0]; // 2-D Array

	template<int register_size, int i2C_addr>
	uint8_t Registers<register_size, i2C_addr>::_regNumber;
#else
	template<int register_size, typename PurposeTag>
	volatile uint8_t Registers<register_size, PurposeTag>::_regArr[register_size];

	template<int register_size, typename PurposeTag>
	uint8_t Registers<register_size, PurposeTag>::_regNumber;
#endif
}
