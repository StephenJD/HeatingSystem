#include "Data_Spell.h"
#include <iostream>
using namespace std;

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace Date_Time;
	//***************************************************
	//              Spell LCD_UI
	//***************************************************
	
	Dataset_Spell::Dataset_Spell(Query & query, VolatileData * volData, I_Record_Interface * parent)
		: Record_Interface(query, volData, parent),
		_startDate(DateTime{ 0 }, ValRange(e_editAll, 0, JUDGEMEMT_DAY.asInt(),0, e_NO_OF_DT_EDIT_POS)) {
		//std::cout << "** Spell " << " Addr:" << std::hex << long long(this) << std::endl;
	}

	I_UI_Wrapper * Dataset_Spell::getField(int fieldID) {
		if (recordID() == -1) return getFieldAt(fieldID, 0);
		switch (fieldID) {
		case e_date: {
			_startDate.val = record().rec().date.asInt();
			return &_startDate;
		}
		default: return 0;
		}
	}

	bool Dataset_Spell::setNewValue(int fieldID, const I_UI_Wrapper * newValue) {
		switch (fieldID) {
		case e_date: {
				_startDate.val = newValue->val;
				auto newDate = DateTime(_startDate.val);;
				//cout << "Insert date: Was: ID:" << dec << (int)recordID() << ": " << record().rec().date.day() << record().rec().date.getMonthStr() << " Now: " << newDate.day() << newDate.getMonthStr() << endl;
				record().rec().date = DateTime(_startDate.val);
				cout << "\nDelete original date: ID:" << dec << (int)record().id();
				auto newRecordID = record().update();
				//cout << "          New RecID: " << (int)newRecordID << ": Date is:  " << record().rec().date.day() << record().rec().date.getMonthStr() << record().rec().date.year() << endl;
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
		auto newSpell = R_Spell{ record().rec() };
		query().insert(&newSpell);
	}
}