#pragma once
#include "..\HardwareInterfaces\Temp_Sensor.h"
#include "..\LCD_UI\I_Record_Interface.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;
	//***************************************************
	//              TempSensor UI Edit
	//***************************************************

	//***************************************************
	//              TempSensor RDB Tables
	//***************************************************

	struct R_TempSensor {
		char name[5];
		uint8_t address; // includes port and active state
		//I2C_Temp_Sensor & obj(int objID) { return HardwareInterfaces::tempSensors[objID]; }
		bool operator < (R_TempSensor rhs) const { return false; }
		bool operator == (R_TempSensor rhs) const { return true; }
	};

#ifdef ZPSIM
	inline std::ostream & operator << (std::ostream & stream, const R_TempSensor & tempSensor) {
		return stream << "TempSensor: " << tempSensor.name << " Addr: " << (int)tempSensor.address;
	}
#endif

	//***************************************************
	//              TempSensor LCD_UI
	//***************************************************

	class UI_TempSensor : public I_Record_Interface
	{
	public:

	};

}
