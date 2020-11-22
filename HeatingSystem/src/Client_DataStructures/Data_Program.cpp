#include "Data_Program.h"
#include "Data_Spell.h"
#include <Date_Time.h>
#include <Conversions.h>


namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              Dataset_Program
	//***************************************************

	bool Dataset_Program::setNewValue(int fieldID, const I_Data_Formatter* newVal) {
		if (fieldID == 0) { //e_id: edit parent spell.program
			Answer_R<R_Spell> spell = query().iterationQ().incrementTableQ()[query().matchArg()];
			spell.rec().programID = static_cast<uint8_t>(newVal->val);
			spell.update();
			return false;
		} else return i_record().setNewValue(fieldID, newVal);
	}

	//***************************************************
	//              RecInt_Program
	//***************************************************

	RecInt_Program::RecInt_Program()
		: _name("", 7) {}

	I_Data_Formatter * RecInt_Program::getField(int fieldID) {
		//if (recordID() == -1) return getFieldAt(fieldID, 0);
		switch (fieldID) {
		case e_name:
			_name = record().rec().name;
			return &_name;
		default: return 0;
		}
	}

	bool RecInt_Program::setNewValue(int fieldID, const I_Data_Formatter * newVal) {
		switch (fieldID) {
		case e_name:
		{
			const StrWrapper* strWrapper(static_cast<const StrWrapper*>(newVal));
			_name = *strWrapper;
			strcpy(record().rec().name, _name.str());
			record().update();
			break;
		}
		}
		return false;
	}
}