#include "I2C_Registers.h"

namespace i2c_registers {
	//int8_t I_Registers::_mutex = 0;
	//int8_t I_Registers::_howMany = 0;
	I2C_Talk* I_Registers::_i2C;

	//bool I_Registers::isLocked(int howMany) {
	//	if (_mutex) {
	//		_howMany = howMany;
	//	}
	//	return _mutex;
	//}

	RegAccess::RegAccess(I_Registers& registers, int regOffset) 
		: _registers(registers)
		, _regOffset(regOffset) 
	{ /*++_registers._mutex;*/ }

//	RegAccess::~RegAccess() {
//		--_registers._mutex;
//		if (_registers._mutex == 0 ) {
//			if (_registers._howMany > 0) {
//				_registers._i2C->receiveFromMaster(_registers._howMany, _registers.regArr() + _registers.regAddr());
//#ifdef DEBUG_REGISTERS
//				logger() << millis() <<  F(" ~C:") << _registers.regAddr() << F(" V:") << _registers.getRegister(_registers.regAddr()) << L_flush;
//#endif
//			}
//			else if (_registers._howMany < 0) {
//				_registers._i2C->write(_registers.regArr() + _registers.regAddr(), -_registers._howMany);
//#ifdef DEBUG_REGISTERS
//				logger() << millis() << F(" ~Q:") << _registers.regAddr() << F(" V:") << _registers.getRegister(_registers.regAddr()) << L_flush;
//#endif
//			}
//			_registers._howMany = 0;
//		}
//	}

}

