#pragma once
#include "Arduino.h"
#include <I2C_Device.h>
#include "RemoteKeypadMaster.h"
#include <I2C_Registers.h>

constexpr uint8_t PROGRAMMER_I2C_ADDR = 0x11;
constexpr uint8_t DS_REMOTE_I2C_ADDR = 0x12;
constexpr uint8_t US_REMOTE_I2C_ADDR = 0x13;
constexpr uint8_t FL_REMOTE_I2C_ADDR = 0x14;

namespace I2C_Recovery { class I2C_Recover; }

namespace OLED_Master_Display {
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
		  remote_register_offset // ini
		, roomTempSensorAddr	// ini
		, roomTemp				// send
		, roomTemp_fraction		// send
		, towelRailRequest		// send
		, hotWaterRequest		// send
		, roomTempRequest		// send/receive
		, roomWarmupTime_m10	// receive
		, towelRailOnTime_m		// receive
		, hotWaterTemp			// receive
		, hotWaterWarmupTime_m10 // If -ve, in 0-60 mins, if +ve in min_10
		, remoteRegister_size
	};

	void setRemoteI2CAddress();
	void sendDataToProgrammer();
	void requestDataFromProgrammer();
	void readTempSensor();
	void begin();
	void displayPage();
	void sleepPage();
	void processKeys();
}