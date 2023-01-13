#pragma once
#include "Arduino.h"
#include <TempSensor.h>
#include <Flag_Enum.h>
#include "RemoteKeypadMaster.h"
#include <I2C_To_MicroController.h>
#include "..\HeatingSystem\src\HardwareInterfaces\A__Constants.h"
#include <U8x8lib.h>

static constexpr uint8_t PROGRAMMER_I2C_ADDR = 0x11;

void moveDisplay(); // defined in .ino

class OLED_Thick_Display : public HardwareInterfaces::I2C_To_MicroController {
public:
	enum Display_Mode {
		RoomTemp
		, TowelRail
		, HotWater
		, NoOfDisplayModes
	};
	enum { R_PROG_WAITING_FOR_REMOTE_I2C_COMS = 1 };

	enum RemoteRegisterName {	// In Slave-Mode, all are received
		R_REMOTE_REG_OFFSET		// [0] ini Always read by programmer to check if it needs ini-sending
		, R_DEVICE_STATE		// [1] ini Flag-Enum {F_EXCHANGE_COMPLETE, F_I2C_NOW, F_ENABLE_KEYBOARD, F_PROGRAMMER_CHANGED_DATA, _NO_OF_FLAGS } // last 4-bits is wake-time/4;
		, R_ROOM_TS_ADDR		// [2] ini
		, R_ROOM_TEMP			// [3] send
		, R_ROOM_TEMP_FRACTION	// [4] send
		, R_REQUESTING_T_RAIL	// [5] send
		, R_REQUESTING_DHW		// [6] send
		, R_REQUESTING_ROOM_TEMP// [7] send/receive
		, R_WARM_UP_ROOM_M10	// [8] receive
		, R_ON_TIME_T_RAIL		// [9] receive
		, R_WARM_UP_DHW_M10		// [10] If -ve, in 0-60 mins, if +ve in min_10
		, R_DISPL_REG_SIZE
	};
	static constexpr uint8_t NO_REG_OFFSET_SET = 170; // 10101010
	enum { e_Auto, e_On, e_Off };
	enum I2C_Flags { F_EXCHANGE_COMPLETE, F_I2C_NOW, F_ENABLE_KEYBOARD, F_PROGRAMMER_CHANGED_DATA, _NO_OF_FLAGS }; // last 4 - bits is wake - time / 4;
	using I2C_Flags_Ref = flag_enum::FE_Ref< I2C_Flags, _NO_OF_FLAGS>;
	using I2C_Flags_Obj = flag_enum::FE_Obj< I2C_Flags, _NO_OF_FLAGS>;
	// New request temps initiated by the programmer are sent by the programmer.
	// In Multi-Master Mode: 
	//		Room temp and requests are sent by the console to the Programmer.
	//		Warmup-times are read by the console from the programmer.
	// In Slave-Mode: 
	//		Requests are read from the console by the Programmer.
	//		Room Temp and Warmup-times are sent to the console by the programmer.
	OLED_Thick_Display(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& my_registers) : OLED_Thick_Display(recover, my_registers, 0, 0, 0) {}
	OLED_Thick_Display(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& my_registers, int other_microcontroller_address, int localRegOffset, int remoteRegOffset);

	void setMyI2CAddress();
	void readTempSensor();
	void begin();
	void displayPage();
	void moveDisplay();
	void sleepPage();
	void processKeys();
#ifdef ZPSIM
	void setKey(HardwareInterfaces::I_Keypad::KeyOperation key);
	char* oledDisplay();
	void setTSaddr(int addr);
#endif
private:
	void clearDisplay();
	void startDisplaySleep();
	void wakeDisplay();
	const uint8_t* getFont(bool bold = false);
	void changeMode(int keyCode);
	void changeValue(int keyCode);
	bool doneI2C_Coms(bool newSecond);
	void refreshRegisters();

	int8_t _sleepRow = 0;
	int8_t _sleepCol = 0;
	int8_t _tempRequest = 0;
	enum State { NO_CHANGE, NEW_T_REQUEST, REFRESH_DISPLAY, WAIT_PROG_ACK };
	State _state = REFRESH_DISPLAY;
	HardwareInterfaces::RemoteKeypadMaster _remoteKeypad{ 50,30 };
	U8X8_SSD1305_128X32_ADAFRUIT_4W_HW_SPI _display;
	Display_Mode _display_mode = RoomTemp;

	HardwareInterfaces::TempSensor _tempSensor;

};	

void moveDisplay();
