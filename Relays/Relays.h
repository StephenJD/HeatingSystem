#pragma once
// This is the Multi-Master Arduino Mini Controller

// ******** NOTE select Arduino Mini Pro 328P 8MHz **********
// See A_SJD_MixerValve_Control.ini
// Arduino Mini Pro must be upgraded with boot-loader to enable watchdog timer.
// File->Preferences->Additional Boards Manager URLs: https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
// Tools->Board->Boards Manager->Install MiniCore
// 1. In Arduino: Load File-> Examples->ArduinoISP->ArduinoISP
// 2. Upload this to any board, including another Mini Pro.
// 3. THEN connect SPI pins to the Mini Pro: MOSI(11)->11, MISO(12)->12, SCK(13)->13, 10->Reset on MiniPro, Vcc and 0c pins.
// 4. Tools->Board->MiniCore Atmega 328
// 5. External 8Mhz clock, BOD disabled, LTO enabled, Variant 328P, UART0.
// 6. Tools->Programmer->Arduino as ISP
// 7. Tools->Burn Bootloader
// 8. Then upload your desired sketch to ordinary MiniPro board. Tools->Board->Arduino Pro Mini / 328/3.3v/8MHz

#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>
#include <Arduino.h>
#include <EEPROM_RE.h>
#include <I2C_Registers.h>
#include <PinObject.h>

extern i2c_registers::I_Registers& rl_registers;

namespace I2C_Recovery {
	class I2C_Recover;
}

class RelaysSlave
{
public:
	enum RL_Device_State { F_I2C_NOW, F_NO_PROGRAMMER, _NO_OF_FLAGS};
	
	enum RelaysSlave_Register_Names {
		// Registers provided by MixValve_Slave
		// Copies of the VOLATILE set provided in Programmer reg-set
		// All registers are single-byte.
		// If the valve is set as a multi-master it reads its own temp sensors.
		// If the valve is set as a slave it obtains the temprrature from its registers.
		// All I2C transfers are initiated by Programmer: Reading status & sending new requests.
		// In multi-master mode, Programmer reads temps from the registers, in slave mode it writes temps to the registers.

		REG_8PORT_IODIR = 0x00 // default all 1's = input
		, REG_8PORT_WAG_STATE = 0x01
		, REG_8PORT_REQ_STATE = 0x02
		, REG_8PORT_PullUp = 0x06
		, REG_8PORT_OPORT = 0x09
		, REG_8PORT_OLAT = 0x0A
		, RL_ALL_REG_SIZE = REG_8PORT_OLAT + 1
	};

	RelaysSlave(I2C_Recovery::I2C_Recover& i2C_recover, EEPROMClass & ep);

	void begin();
	void setPorts();
	void test();
	i2c_registers::RegAccess registers() const {return { rl_registers, 0 };}

private:
	//friend class TestRelays;
	EEPROMClass * _ep;
	HardwareInterfaces::Pin_Wag _ports[8];
	uint32_t _lastChangeTime = 0;
};