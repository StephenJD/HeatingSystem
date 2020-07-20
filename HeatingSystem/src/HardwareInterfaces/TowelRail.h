#pragma once
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino

//***************************************************
//              TowelRail Dynamic Class
//***************************************************

namespace Assembly {
	class TemperatureController;
}

namespace HardwareInterfaces {
	class UI_Bitwise_Relay;
	class UI_TempSensor;
	class MixValveController;

	class TowelRail : public LCD_UI::VolatileData
	{
	public:
		TowelRail() = default;
		// Queries
		RelationalDatabase::RecordID record() const { return _recordID; }
		uint8_t relayPort() const;
		uint16_t timeToGo() const;

		// Modifier
		void initialise(int towelRailID, UI_TempSensor & callTS, UI_Bitwise_Relay & callRelay, uint16_t onTime,  Assembly::TemperatureController & temperatureController, MixValveController & mixValveController);
		bool check();
	private:
		RelationalDatabase::RecordID _recordID = 0;
		UI_TempSensor * _callTS = 0;
		UI_Bitwise_Relay * _relay = 0;
		MixValveController * _mixValveController;
		//int8_t _maxFlowTemp = 0;
		Assembly::TemperatureController * _temperatureController;

		bool rapidTempRise() const;
		uint8_t sharedZoneCallTemp() const;
		bool setFlowTemp();

		uint16_t _onTime = 0;
		uint8_t _callFlowTemp = 0;
		mutable uint16_t _timer = 0;
		mutable uint8_t _prevTemp = 0;

	};
}

