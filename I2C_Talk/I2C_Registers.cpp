#include "I2C_Registers.h"

namespace i2c_registers {
	I2C_Talk* I_Registers::_i2C;

	RegAccess::RegAccess(I_Registers& registers, int regOffset) 
		: _registers(registers)
		, _regOffset(regOffset) 
	{}
}

