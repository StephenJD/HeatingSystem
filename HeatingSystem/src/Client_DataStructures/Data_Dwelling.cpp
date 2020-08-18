#include "Data_Dwelling.h"

namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              Dwelling UI Edit
	//***************************************************

	//***************************************************
	//              Dwelling Dynamic Class
	//***************************************************

	//***************************************************
	//              Dwelling LCD_UI
	//***************************************************

	Dataset_Dwelling::Dataset_Dwelling(Query & query, VolatileData * volData, I_Record_Interface * parent)
		: Record_Interface(query, volData, parent), _name("", 7) {
	}

	I_Data_Formatter * Dataset_Dwelling::getField(int fieldID) {
		if (recordID()==-1) return getFieldAt(fieldID, 0);
		switch (fieldID) {
		case e_name:
			_name = record().rec().name;
			return &_name;
		default: return 0;
		}
	}

	bool Dataset_Dwelling::setNewValue(int fieldID, const I_Data_Formatter * val) {
		switch (fieldID) {
		case e_name: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(val));
			_name = *strWrapper;
			strcpy(record().rec().name, _name.str());
			setRecordID(record().update());
			break;	}
		default:;
		}
		return false;
	}
}
