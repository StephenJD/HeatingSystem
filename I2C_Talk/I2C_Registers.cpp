#include "I2C_Registers.h"

namespace i2c_registers {
	bool I_Registers::_mutex = false;

	RegAccess::RegAccess(I_Registers& registers, int regOffset) 
		: _registers(registers)
		, _regOffset(regOffset) 
	{
		if (_registers._mutex != true) {
			_amOwner = true;
			_registers._mutex = true;
		}
	}
}

