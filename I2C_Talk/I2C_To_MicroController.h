#pragma once
#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <I2C_Registers.h>

void ui_yield();

namespace HardwareInterfaces {
	/// <summary>
	/// For accessing another microcontoller via I2C.
	/// Maps local registers via getReg/setReg for use with the remote device
	/// </summary>
	class I2C_To_MicroController : public I_I2Cdevice_Recovery {
	public:
		I2C_To_MicroController(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers) : I2C_To_MicroController(recover, prog_registers, 0, 0, 0) {}
		I2C_To_MicroController(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers, int address, int regOffset, unsigned long* timeOfReset_mS);
		// Queries
		uint8_t getRawReg(int reg) const { return _localRegisters->getRegister(reg); }
		uint8_t getReg(int reg) const { return getRawReg(_regOffset + reg); }
		void waitForWarmUp() const;

		// Modifiers
		I2C_Talk_ErrorCodes::Error_codes testDevice() override;
		void initialise(int address, int regOffset, unsigned long& timeOfReset_mS);
		void setRawReg(int reg, uint8_t value) { _localRegisters->setRegister( reg, value); }
		void setReg(int reg, uint8_t value) { setRawReg(_regOffset + reg, value); }
		bool updateReg(int reg, uint8_t value) { return _localRegisters->updateRegister(_regOffset + reg, value); }
		uint8_t* regPtr(int reg) { return _localRegisters->reg_ptr(_regOffset + reg); }

	protected:
		uint8_t _regOffset = 0;

	private:
		i2c_registers::I_Registers* _localRegisters;
		mutable unsigned long* _timeOfReset_mS = 0;
		static constexpr int STOP_MARGIN_TIMEOUT = 15;
	};
}
