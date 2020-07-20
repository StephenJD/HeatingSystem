#pragma once
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino

//***************************************************
//              Relay Dynamic Class
//***************************************************

//namespace Assembly {
//	class TemperatureController;
//}
//
//namespace HardwareInterfaces {
//	class UI_Bitwise_Relay;
//	class UI_TempSensor;
//	class MixValveController;
//
//	class Relay : public LCD_UI::VolatileData {
//	public:
//		Relay() = default;
//		// Queries
//		RelationalDatabase::RecordID record() const { return _recordID; }
//		uint8_t relayPort() const;
//		uint16_t timeToGo() const;
//
//		// Modifier
//		void initialise(int towelRailID, UI_TempSensor & callTS, UI_Bitwise_Relay & callRelay, uint16_t onTime, Assembly::TemperatureController & temperatureController, MixValveController & mixValveController);
//		bool check();
//	private:
//		RelationalDatabase::RecordID _recordID = 0;
//		//int8_t _maxFlowTemp = 0;
//		Assembly::TemperatureController * _temperatureController;
//
//	};
//}