#pragma once
#include "..\LCD_UI\I_Record_Interface.h" 


namespace HardwareInterfaces {
	class I2C_Temp_Sensor;
	class Bitwise_Relay;

	class BackBoiler : public LCD_UI::VolatileData {
	public:
		BackBoiler(I2C_Temp_Sensor & flowTS, I2C_Temp_Sensor & thrmStrTS, Bitwise_Relay & relay);
		bool check(); // returns true if ON
	private:
		I2C_Temp_Sensor * _flowTS;
		I2C_Temp_Sensor * _thrmStrTS;
		Bitwise_Relay * _relay;
		uint8_t _minFlowTemp = 45;
		uint8_t _minTempDiff = 2;
	};

}