#include "Data_Relay.h"
#include "..\..\..\Conversions\Conversions.h"
#include "..\LCD_UI\UI_FieldData.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;     
	using namespace HardwareInterfaces;

	//***************************************************
	//              Dataset_Relay
	//***************************************************

	Dataset_Relay::Dataset_Relay(Query & query)
		: Record_Interface(query, 0, 0)
		, _name("", 6) {}

	I_UI_Wrapper * Dataset_Relay::getField(int fieldID) {
		static IntWrapper status_wrappper;
		if (recordID() == -1 || record().status() != TB_OK) return 0;
		switch (fieldID) {
		case e_name:
			_name = record().rec().name;
			return &_name;
		case e_state:
			return &status_wrappper;
		default: return 0;
		}
	}

}