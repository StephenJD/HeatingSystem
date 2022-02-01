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

	enum RemoteRegisterName {	// In Slave-Mode, all are received
		R_DISPL_REG_OFFSET		// ini
		, R_MODE			// ini { e_LCD, e_MASTER, e_ENABLE_KEYBOARD, e_DATA_CHANGED, e_NO_OF_FLAGS };
		, R_ROOM_TS_ADDR		// ini
		, R_ROOM_TEMP			// send
		, R_ROOM_TEMP_FRACTION	// send
		, R_REQUESTING_T_RAIL	// send
		, R_REQUESTING_DHW		// send
		, R_REQUESTING_ROOM_TEMP	// send/receive
		, R_WARM_UP_ROOM_M10	// receive
		, R_ON_TIME_T_RAIL		// receive
		, R_WARM_UP_DHW_M10		// If -ve, in 0-60 mins, if +ve in min_10
		, R_DISPL_REG_SIZE
	};

	enum { e_Auto, e_On, e_Off };
	enum DisplayModes { e_LCD, e_MASTER, e_ENABLE_KEYBOARD, e_DATA_CHANGED, e_NO_OF_FLAGS, NO_REG_OFFSET_SET = 255 };
	using ModeFlags = flag_enum::FE< DisplayModes, e_NO_OF_FLAGS>;
	// New request temps initiated by the programmer are sent by the programmer.
	// In Multi-Master Mode: 
	//		Room temp and requests are sent by the console to the Programmer.
	//		Warmup-times are read by the console from the programmer.
	// In Slave-Mode: 
	//		Requests are read from the console by the Programmer.
	//		Room Temp and Warmup-times are sent to the console by the programmer.
	OLED_Thick_Display(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& my_registers) : OLED_Thick_Display(recover, my_registers, 0, 0, 0) {}
	OLED_Thick_Display(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& my_registers, int address, int regOffset, unsigned long* timeOfReset_mS);

	void setMyI2CAddress();
	void sendDataToProgrammer(int reg);
	void readTempSensor();
	void begin();
	void displayPage();
	void moveDisplay();
	void sleepPage();
	void processKeys();
#ifdef ZPSIM
	void setKey(HardwareInterfaces::I_Keypad::KeyOperation key);
	char* oledDisplay();
#endif
private:
	void clearDisplay();
	void startDisplaySleep();
	void wakeDisplay();
	const uint8_t* getFont(bool bold = false);
	void changeMode(int keyCode);
	void changeValue(int keyCode);
	uint8_t requestRegisterOffsetFromProgrammer();
	void refreshRegisters();

	int8_t _sleepRow = -1;
	int8_t _sleepCol = 0;
	int8_t _tempRequest = 0;
	bool _dataChanged = false;
	HardwareInterfaces::RemoteKeypadMaster _remoteKeypad{ 50,30 };
	U8X8_SSD1305_128X32_ADAFRUIT_4W_HW_SPI _display;
	Display_Mode _display_mode = RoomTemp;

	HardwareInterfaces::TempSensor _tempSensor;

};	

void moveDisplay();
