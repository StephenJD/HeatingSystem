#pragma once
#include "Arduino.h"
#include <I2C_Device.h>
#include "RemoteKeypadMaster.h"
#include <I2C_Registers.h>


namespace I2C_Recovery { class I2C_Recover; }

namespace OLED_Master_Display {
	constexpr uint8_t PROGRAMMER_I2C_ADDR = 0x11;
	constexpr uint8_t US_CONSOLE_I2C_ADDR = 0x12;
	constexpr uint8_t DS_CONSOLE_I2C_ADDR = 0x13;
	constexpr uint8_t FL_CONSOLE_I2C_ADDR = 0x14;

	constexpr auto SLEEP_MOVE_PERIOD_mS = 500;
	constexpr auto SCREEN_WIDTH = 128;  // OLED display width, in pixels
	constexpr auto SCREEN_HEIGHT = 32; // OLED display height, in pixels
	constexpr auto FONT_WIDTH = 8;
	// Declaration for SSD1306 display connected using software SPI (default case):
	constexpr auto OLED_MOSI = 11;  // DIn
	constexpr auto OLED_CLK = 13;
	constexpr auto OLED_DC = 9;
	constexpr auto OLED_CS = 8;
	constexpr auto OLED_RESET = 10;

	enum Display_Mode {
		RoomTemp
		, TowelRail
		, HotWater
		, NoOfDisplayModes
	};

	enum RemoteRegisterName {
		  R_DISPL_REG_OFFSET	// ini
		, R_ROOM_TS_ADDR		// ini
		, R_ROOM_TEMP			// send
		, R_ROOM_TEMP_FRACTION	// send
		, R_REQUESTING_T_RAIL	// send
		, R_REQUESTING_DHW		// send
		, R_REQUESTED_ROOM_TEMP	// send/receive
		, R_WARM_UP_ROOM_M10	// receive
		, R_ON_TIME_T_RAIL		// receive
		, R_WARM_UP_DHW_M10		// If -ve, in 0-60 mins, if +ve in min_10
		, R_DISPL_REG_SIZE
	};

	enum { e_Auto, e_On, e_Off, e_ModeIsSet };

	// Room temp and requests are sent by the console to the Programmer.
	// The programmer does not read them from the console.
	// New request temps initiated by the programmer are sent by the programmer and not read by the console.
	// Warmup-times are read by the console from the programmer.

	void setRemoteI2CAddress();
	void sendDataToProgrammer(int reg);
	void requestDataFromProgrammer();
	void readTempSensor();
	void begin();
	void displayPage();
	void sleepPage();
	void processKeys();
#ifdef ZPSIM
	void setKey(HardwareInterfaces::I_Keypad::KeyOperation key);
	char* oledDisplay();
#endif
}