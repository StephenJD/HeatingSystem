#pragma once
#include "..\HardwareInterfaces\MixValveController.h"
#include "..\LCD_UI\I_Record_Interface.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;
	//***************************************************
	//              MixValveController UI Edit
	//***************************************************

	//***************************************************
	//              MixValveController RDB Tables
	//***************************************************

	struct R_MixValveControl {
		char name[5];
		uint8_t address;
		//MixValveController & obj(int objID) { return *HardwareInterfaces::mixValveController; }
	};

	//***************************************************
	//              MixValveController LCD_UI
	//***************************************************

	//class UI_MixValveController : public I_Record_Interface
	//{
	//public:

	//};
}

