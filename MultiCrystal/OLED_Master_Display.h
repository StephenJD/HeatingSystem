#pragma once
#include "Arduino.h"
#include <I2C_Device.h>
#include "RemoteKeypadMaster.h"

namespace I2C_Recovery { class I2C_Recover; }

namespace OLED_Master_Display {
	constexpr auto SLEEP_MOVE_PERIOD_mS = 500;

	enum Display_Mode {
		RoomTemp
		, TowelRail
		, HotWater
		, NoOfDisplayModes
	};

	class Registers {
		public:
			enum Reg {roomTempRequest
				, roomTemp
				, roomWarmupTime
				, towelRailRequest
				, towelRailOnTime
				, hotWaterRequest
				, hotWaterTemp
				, hotWaterWarmupTime
				, reg_size = hotWaterWarmupTime + 3
			};
			void setRegister(uint8_t reg, uint8_t value) { _regArr[reg] = value; }
			void modifyRegister(Reg reg, uint8_t increment) { _regArr[reg] += increment; }
			uint8_t getRegister(uint8_t reg) { return _regArr[reg]; }
			bool dataChanged = false;
		private:
			uint8_t _regArr[6] = { 19, 12, 0, 60, 0, 45 };
			uint16_t _hotWaterWarmupTime = 360; // in mins
	};

	void begin();
	void displayPage();
	void sleepPage();
	void processKeys();
}