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
		enum : uint8_t { DEVICE_CAN_WRITE = 0x0F, DEVICE_IS_FINISHED = 0xF0, DATA_READY = 0x40, DATA_READ = 0x80, EXCHANGE_COMPLETE = 0xC0 };
		I2C_To_MicroController(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& local_registers) : I2C_To_MicroController(recover, local_registers, 0, 0, 0) {}
		I2C_To_MicroController(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& local_registers, int address, int localRegOffset, int remoteRegOffset);
		// Queries

		// Modifiers
		I2C_Talk_ErrorCodes::Error_codes testDevice() override;
		void initialise(int address, int localRegOffset, int remoteRegOffset);
		i2c_registers::RegAccess registers() const { return { *_localRegisters,_localRegOffset }; }
		i2c_registers::RegAccess rawRegisters() const { return { *_localRegisters,0 }; }
		I2C_Talk_ErrorCodes::Error_codes writeOnly_RegValue(int reg, uint8_t value);
		I2C_Talk_ErrorCodes::Error_codes writeRegValue(int reg, uint8_t value);
		I2C_Talk_ErrorCodes::Error_codes writeReg(int reg);
		I2C_Talk_ErrorCodes::Error_codes writeRegSet(int reg, int noToWrite);
		I2C_Talk_ErrorCodes::Error_codes readVerifyReg(int reg);
		I2C_Talk_ErrorCodes::Error_codes readRegSet(int reg, int noToRead);
		I2C_Talk_ErrorCodes::Error_codes readRegVerifyValue(int reg, uint8_t& value);
		I2C_Talk_ErrorCodes::Error_codes getInrangeVal(int regNo, int minVal, int maxVal);
		bool handShake_send(uint8_t remoteRegNo, uint8_t data);
		bool give_I2C_Bus(i2c_registers::RegAccess localReg, uint8_t localRegNo, uint8_t remoteRegNo, uint8_t i2c_status);
		bool wait_DevicesToFinish(i2c_registers::RegAccess reg, int regNo);

	protected:
		uint8_t _localRegOffset = 0;
		uint8_t _remoteRegOffset = 0;
	private:
		i2c_registers::I_Registers* _localRegisters;
		static constexpr int STOP_MARGIN_TIMEOUT = 15;
	};
}
