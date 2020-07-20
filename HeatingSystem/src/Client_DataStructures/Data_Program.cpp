#include "Data_Program.h"
#include "Data_Spell.h"
#include <Date_Time.h>
#include <Conversions.h>


namespace client_data_structures {
	using namespace LCD_UI;
	//***************************************************
	//              Program UI Edit
	//***************************************************

	//***************************************************
	//              Program Dynamic Class
	//***************************************************

	//***************************************************
	//              Program RDB Tables
	//***************************************************

	//***************************************************
	//              Program LCD_UI
	//***************************************************

	Dataset_Program::Dataset_Program(Query & query, VolatileData * volData, I_Record_Interface * parent)
		: Record_Interface(query, volData,parent), _name("", 7) {
	}

	//int Dataset_Program::resetCount() {
	//	return Record_Interface::resetCount();
	//}

	I_UI_Wrapper * Dataset_Program::getField(int fieldID) {
		if (recordID() == -1) return getFieldAt(fieldID, 0);
		switch (fieldID) {
		//case e_id:

		case e_name:
			_name = record().rec().name;
			return &_name;
		default: return 0;
		}
	}

	bool Dataset_Program::setNewValue(int fieldID, const I_UI_Wrapper * newVal) {
		switch (fieldID) {
		case e_name: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newVal));
			_name = *strWrapper;
			strcpy(record().rec().name, _name.str());
			setRecordID(record().update());
			break; }
		}
		return false;
	}
}