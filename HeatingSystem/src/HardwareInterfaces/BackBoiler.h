#pragma once
#include <HeatingSystem/src/LCD_UI/I_Record_Interface.h>


namespace HardwareInterfaces {
	class I2C_Temp_Sensor;
	class Relay;

	class BackBoiler : public LCD_UI::VolatileData {
	public:
		BackBoiler(I2C_Temp_Sensor & flowTS, I2C_Temp_Sensor & thrmStrTS, Relay & relay);
		bool check(); // returns true if ON
	private:
		I2C_Temp_Sensor * _flowTS;
		I2C_Temp_Sensor * _thrmStrTS;
		Relay * _relay;
		uint8_t _minFlowTemp = 45;
		uint8_t _minTempDiff = 2;
	};

}