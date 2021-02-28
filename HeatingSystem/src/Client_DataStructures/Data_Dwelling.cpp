#include "Data_Dwelling.h"

namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              RecInt_Dwelling
	//***************************************************

	RecInt_Dwelling::RecInt_Dwelling()
		: _name("", 7) {
	}

	I_Data_Formatter * RecInt_Dwelling::getField(int fieldID) {
		switch (fieldID) {
		case e_name:
			_name = answer().rec().name;
			return &_name;
		default: return 0;
		}
	}

	bool RecInt_Dwelling::setNewValue(int fieldID, const I_Data_Formatter * val) {
		switch (fieldID) {
		case e_name: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(val));
			_name = *strWrapper;
			strcpy(answer().rec().name, _name.str());
			break;	
		}
		}
		return false;
	}
}
