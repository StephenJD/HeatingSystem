#include "Data_Spell.h"
#include <Clock.h>

#ifdef ZPSIM
	#include <iostream>
	using namespace std;
#endif

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace Date_Time;

	//***************************************************
	//              Dataset_Spell
	//***************************************************

	Dataset_Spell::Dataset_Spell(I_Record_Interface& recordInterface, Query& query, Dataset* dwelling) 
		: Dataset(recordInterface, query, dwelling)
	{}

	bool Dataset_Spell::setNewValue(int fieldID, const I_Data_Formatter* newValue) {
		auto & spell = static_cast<Answer_R<R_Spell>&>(i_record().answer());
		switch (fieldID) {
		case e_date:
		{
			auto startDate = newValue->val;
			auto newDate = DateTime(startDate);
			if (spell.rec().date <= clock_().now() && newDate > clock_().now()) {
				insertNewData();
				logger() << "Insert Spell\n";
			}

			for (Answer_R<R_Spell> spell : query()) {
				if (spell.rec().date == newDate) {
					deleteData();
					logger() << "Delete Spell " << newDate << L_endl;
					return false;
				}
			}

			spell.rec().date = DateTime(startDate);
			auto newRecordID = spell.update();
			if (newRecordID != ds_recordID()) {
				query().moveTo(_recSel, newRecordID);
				setDS_RecordID(newRecordID);
				return true;
			}
			break;
		}
		case e_progID:
		{
			spell.rec().programID = (RecordID)newValue->val;
			auto newRecordID = spell.update();
			if (newRecordID != ds_recordID()) {
				setDS_RecordID(newRecordID);
				return true;
			}
			break;
		}
		}
		return false;
	}

	//void Dataset_Spell::insertNewData() {
	//	query().insert(static_cast<Answer_R<R_Spell>>(record()).rec());
	//}

	//***************************************************
	//              RecInt_Spell
	//***************************************************

	RecInt_Spell::RecInt_Spell()
		: _startDate(DateTime{ 0 }, ValRange(e_editAll, 0, JUDGEMEMT_DAY.asInt(),0, e_NO_OF_DT_EDIT_POS)) {}

	I_Data_Formatter * RecInt_Spell::getField(int fieldID) {
		switch (fieldID) {
		case e_date: {
			_startDate.val = answer().rec().date.asInt();
			return &_startDate;
		}
		default: return 0;                                                           
		}
	}
}