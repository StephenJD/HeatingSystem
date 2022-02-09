#pragma once
#include <Arduino.h>
#include <I2C_Talk.h>

//#define DEBUG_REGISTERS
//#ifdef DEBUG_REGISTERS
//#include <Logging.h>
//namespace arduino_logger {
//	Logger& logger();
//}
//using namespace arduino_logger;
//#endif

namespace i2c_registers {

	struct Defaut_Tag_None {};
	/// <summary>
	/// Registers are in static-storage and thus initialised to zero
	/// </summary>
	class I_Registers {
	public:
		//static bool isQueued() { return _howMany; }
		//static bool isLocked(int howMany);
	private:
		uint8_t getRegister(int reg) const { return const_cast<I_Registers*>(this)->regArr()[reg]; }
		void setRegister(int reg, uint8_t value) {regArr()[reg] = value; }
		bool updateRegister(int reg, uint8_t value) { 
			bool hasChanged = regArr()[reg] != value; 
			if(hasChanged) regArr()[reg] = value;
			return hasChanged;
		}
		void addToRegister(int reg, uint8_t increment) { regArr()[reg] += increment; }
		volatile uint8_t* reg_ptr(int reg) { return regArr() + reg; }
	private:
		friend class RegAccess;
		virtual volatile uint8_t* regArr() = 0;
		virtual uint8_t& regAddr() = 0;
		//static int8_t _mutex;
		//static int8_t _howMany;
	protected:
		static I2C_Talk* _i2C;
	};

	/// <summary>
	/// This was a failed attempt to protect registers from I2C comms during updates
	/// It requires clock-stretching the I2C which is very tricky and non-portable
	/// delaying the reading / writing done inside receiveI2C/requestI2C doesn't work, as those functions
	/// assume that the read/write was already sucessful and so does not produce a clock-stretch.
	/// </summary>
	class RegAccess {
	public:
		RegAccess(I_Registers& registers, int regOffset = 0);
		//~RegAccess();
		uint8_t get(int reg) const { return _registers.getRegister(_regOffset + reg); }
		void set(int reg, uint8_t value) { return _registers.setRegister(_regOffset + reg, value); }
		bool update(int reg, uint8_t value) { return _registers.updateRegister(_regOffset + reg, value); }
		void add(int reg, uint8_t increment) { _registers.addToRegister(_regOffset + reg, increment); }
		volatile uint8_t* ptr(int reg) { return _registers.reg_ptr(_regOffset + reg); }
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
		int noOfRegisters() { return register_size; }

		// Called when data is sent by a Master, telling this slave how many bytes have been sent.
		// receiveI2C could start during a RegisterUpdate/Read, but RegisterUpdate/Read cannot start during receiveI2C.
		// The bytes have already been sent when this gets called.
		// Slave can hold CLK low until it is ready to receive data.
		static void receiveI2C(int howMany) {
			//if (!isQueued()) { // stops it working!
			//if (true) {
				uint8_t regNo;
				if (_i2C->receiveFromMaster(1, &regNo)) _regAddr = regNo; // first byte is reg-address
				//logger() << millis() << F(" CR:") << _regAddr << L_endl;				
			//if (!isQueued()) { // stops it working!
				if (--howMany) {
						if (_regAddr + howMany > register_size) howMany = register_size - _regAddr;
						//if (true) {
						//if (!isLocked(howMany)) { // doesn't like isLocked()
							_i2C->receiveFromMaster(howMany, _regArr + _regAddr);
							//logger() << millis() << F(" C:") << _regAddr << F(" V:") << *(_regArr + _regAddr) << L_endl;
						//}
						//else logger() << millis() << F(" LC:") << _regAddr << L_endl;
				}
			//} 
			//else {
			//	logger() << millis() << F(" XCR:") << L_endl;
			//}
		}

		// Called when data is requested by a Master from this slave, reading from the last register address sent.
		// requestI2C could start during a RegisterUpdate, but RegisterUpdate cannot start during requestI2C.
		// Writes bytes to the transmit buffer. When it returns the bytes are sent.
		static void requestI2C() { 
			// The Master will send a NACK when it has received the requested number of bytes, so we should offer as many as could be requested.
			//if (!isQueued()) { // OK
				int bytesAvaiable = register_size - _regAddr;
				if (bytesAvaiable > 32) bytesAvaiable = 32;
				//if (!isLocked(-bytesAvaiable)) { // OK
				//if (true) {
					_i2C->write(_regArr + _regAddr, bytesAvaiable);
					//logger() << millis() << F(" Q:") << _regAddr << F(" V:") << *(_regArr + _regAddr) << L_endl;
				//}
				//else logger() << millis() << F(" LQ:") << _regAddr << L_endl;
			//} 
			//else {
			//	logger() << millis() << F(" XQ:") << L_endl;
			//}
		}
	private:
		volatile uint8_t* regArr() override {return _regArr;}
		uint8_t& regAddr() override {return _regAddr;}
#ifdef ZPSIM
		volatile static uint8_t* _regArr;
#else
		volatile static uint8_t _regArr[register_size];
#endif
		static uint8_t _regAddr; // the register address sent in the request
	};

// Static Definitions
#ifdef ZPSIM
#include "Wire.h"
	template<int register_size, int i2C_addr>
	volatile uint8_t* Registers<register_size, i2C_addr>::_regArr = & TwoWire::i2CArr[i2C_addr][0]; // 2-D Array

	template<int register_size, int i2C_addr>
	uint8_t Registers<register_size, i2C_addr>::_regAddr;
#else
	template<int register_size, typename PurposeTag>
	volatile uint8_t Registers<register_size, PurposeTag>::_regArr[register_size];

	template<int register_size, typename PurposeTag>
	uint8_t Registers<register_size, PurposeTag>::_regAddr;
#endif
}
