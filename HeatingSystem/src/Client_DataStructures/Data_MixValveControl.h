#pragma once
#include "..\HardwareInterfaces\MixValveController.h"
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\LCD_UI\I_Data_Formatter.h"

namespace HardwareInterfaces {
	class MixValveController;
}

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

	//***************************************************
	//              MixValveController RDB Tables
	//***************************************************

	struct R_MixValveControl {
		char name[5];
		RecordID flowTS_addr;
		RecordID storeTempSens;
		bool operator < (R_MixValveControl rhs) const { return false; }
		bool operator == (R_MixValveControl rhs) const { return true; }
	};

inline arduino_logger::Logger & operator << (arduino_logger::Logger & stream, const R_MixValveControl & mixValve) {
	return stream << F("R_MixValveControl ") << mixValve.name;
}

//***************************************************
//              RecInt_MixValveController
//***************************************************

	/// <summary>
	/// DB Interface to all TowelRail Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// A single object may be used to stream and edit any of the fields via getFieldAt
	/// </summary>
	class RecInt_MixValveController : public Record_Interface<R_MixValveControl> {
	public:
		enum streamable { e_multiMode, e_pos, e_flowTemp, e_reqTemp, e_state	};
		RecInt_MixValveController(MixValveController* mixValveArr);
		MixValveController& runTimeData() override { return _mixValveArr[answer().id()]; }
		I_Data_Formatter* getField(int _fieldID) override;
		bool setNewValue(int _fieldID, const I_Data_Formatter* val) override;
	private:
		char _name0[6] = { 0 };
		char _name1[6] = { 0 };
		const char* _options[2];
		MixValveController* _mixValveArr;
		OptionsWrapper _name_mode;
		IntWrapper _pos;
		IntWrapper _isTemp;
		IntWrapper _reqTemp;
		StrWrapper _state;
	};
}

