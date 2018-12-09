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
		//record() = this->query().firstMatch();
		//std::cout << "** Dwelling " << " Addr:" << std::hex << long long(this) << std::endl;
	}

	I_UI_Wrapper * Dataset_Dwelling::getField(int fieldID) {
		if (recordID()==-1) return getFieldAt(fieldID, 0);
		switch (fieldID) {
		case e_name:
			_name = record().rec().name;
			return &_name;
		default: return 0;
		}
	}

	bool Dataset_Dwelling::setNewValue(int fieldID, const I_UI_Wrapper * val) {
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
