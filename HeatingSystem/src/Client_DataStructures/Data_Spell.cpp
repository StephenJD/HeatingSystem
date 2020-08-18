#include "Data_Spell.h"

#ifdef ZPSIM
	#include <iostream>
	using namespace std;
#endif

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace Date_Time;
	//***************************************************
	//              Spell LCD_UI
	//***************************************************
	
	Dataset_Spell::Dataset_Spell(Query & query, VolatileData * volData, I_Record_Interface * parent)
		: Record_Interface(query, volData, parent),
		_startDate(DateTime{ 0 }, ValRange(e_editAll, 0, JUDGEMEMT_DAY.asInt(),0, e_NO_OF_DT_EDIT_POS)) {
	}

	I_Data_Formatter * Dataset_Spell::getField(int fieldID) {
		if (recordID() == -1) return getFieldAt(fieldID, 0);
		switch (fieldID) {
		case e_date: {
			_startDate.val = record().rec().date.asInt();
			return &_startDate;
		}
		default: return 0;
		}
	}

	bool Dataset_Spell::setNewValue(int fieldID, const I_Data_Formatter * newValue) {
		switch (fieldID) {
		case e_date: {
				_startDate.val = newValue->val;
				auto newDate = DateTime(_startDate.val);;
				record().rec().date = DateTime(_startDate.val);
				auto newRecordID = record().update();
				if (newRecordID != recordID()) {
					query().moveTo(_recSel, newRecordID);
					setRecordID(newRecordID);
					return true;
				}
				break; 
			}
		case e_progID: {
				record().rec().programID = (RecordID)newValue->val;
				auto newRecordID = record().update();
				if (newRecordID != recordID()) {
					setRecordID(newRecordID);
					return true;
				}
				break; 
			}
		}
		return false;
	}

	void Dataset_Spell::insertNewData() {
		query().insert(record().rec());
	}
}