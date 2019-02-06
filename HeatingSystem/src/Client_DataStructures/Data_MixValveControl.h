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
		RecordID flowTempSens;
		RecordID storeTempSens;
		bool operator < (R_MixValveControl rhs) const { return false; }
		bool operator == (R_MixValveControl rhs) const { return true; }
	};

#ifdef ZPSIM
inline std::ostream & operator << (std::ostream & stream, const R_MixValveControl & profile) {
	return stream << "R_MixValveControl";
}
#endif
	//***************************************************
	//              MixValveController LCD_UI
	//***************************************************

	//class UI_MixValveController : public I_Record_Interface
	//{
	//public:

	//};
}

