#pragma once
#include "..\LCD_UI\I_Record_Interface.h" 

namespace HardwareInterfaces {
	class UI_TempSensor;
	class UI_Bitwise_Relay;

	class BackBoiler : public LCD_UI::VolatileData {
	public:
		BackBoiler(UI_TempSensor & flowTS, UI_TempSensor & thrmStrTS, UI_Bitwise_Relay & relay);
		bool check(); // returns true if ON
		bool isOn() const;
		bool isWarm() const;
	private:
		UI_TempSensor * _flowTS;
		UI_TempSensor * _thrmStrTS;
		UI_Bitwise_Relay * _relay;
		uint8_t _minFlowTemp = 45;
		uint8_t _minTempDiff = 2;
	};
}