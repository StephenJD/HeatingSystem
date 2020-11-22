#pragma once
#include "..\HardwareInterfaces\MixValveController.h"
#include "..\LCD_UI\I_Record_Interface.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

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

inline Logger & operator << (Logger & stream, const R_MixValveControl & mixValve) {
	return stream << F("R_MixValveControl ") << mixValve.name;
}

//***************************************************
//              RecInt_MixValveController
//***************************************************

//class RecInt_MixValveController : public Record_Interface<R_MixValveControl> {

}

