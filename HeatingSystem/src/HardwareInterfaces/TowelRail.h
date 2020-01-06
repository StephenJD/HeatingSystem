#pragma once
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino
//#include "Date_Time.h"

//***************************************************
//              TowelRail Dynamic Class
//***************************************************

namespace Assembly {
	class TemperatureController;
}

namespace HardwareInterfaces {
	class Relay;
	class I2C_Temp_Sensor;
	class MixValveController;

	class TowelRail : public LCD_UI::VolatileData
	{
	public:
		TowelRail() = default;
		// Queries
		RelationalDatabase::RecordID record() const { return _recordID; }
		
		bool isCallingHeat() const;

		// Modifier
		void initialise(int towelRailID, I2C_Temp_Sensor & callTS, Relay & callRelay, Assembly::TemperatureController & temperatureController, MixValveController & mixValveController, int8_t maxFlowTemp);
	private:
		RelationalDatabase::RecordID _recordID = 0;
		I2C_Temp_Sensor * _callTS = 0;
		Relay * _relay = 0;
		MixValveController * _mixValveController;
		int8_t _maxFlowTemp = 0;
		Assembly::TemperatureController * _temperatureController;

		bool rapidTempRise() const;
		//uint8_t sharedZoneCallTemp(uint8_t callRelay) const;
		bool setFlowTemp();

		uint8_t _callFlowTemp = 0;
		mutable uint16_t _timer = 0;
		mutable uint8_t _prevTemp = 0;

	};
}

