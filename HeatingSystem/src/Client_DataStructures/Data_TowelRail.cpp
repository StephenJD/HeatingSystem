#include "Data_TowelRail.h"
#include "..\..\..\Conversions\Conversions.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\HardwareInterfaces\TowelRail.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;
	using namespace Date_Time;       
	using namespace HardwareInterfaces;

	//***************************************************
	//             Dataset_TowelRail
	//***************************************************

	Dataset_TowelRail::Dataset_TowelRail(Query & query, VolatileData * runtimeData, I_Record_Interface * parent)
		: Record_Interface(query, runtimeData, parent),
		_name("", 8)
		, _onTemp(90, ValRange(e_editAll, 10, 60))
		, _minutesOn(0, ValRange(e_fixedWidth | e_editAll, 0, 240))
	{}

	I_Data_Formatter * Dataset_TowelRail::getField(int fieldID) {
		if (recordID() == -1 || record().status() != TB_OK) return 0;
		switch (fieldID) {
		case e_name:
			_name = record().rec().name;
			return &_name;
		case e_onTemp:
			_onTemp.val = record().rec().onTemp;
			return &_onTemp;
		case e_minutesOn:
			_minutesOn.val = record().rec().minutes_on;
			return &_minutesOn;
		case e_secondsToGo:
		{
			towelRail(recordID()).check();
			_secondsToGo.val = towelRail(recordID()).timeToGo();
			if (_secondsToGo.val) logger() << L_time << "TowelRail " << recordID() << " timeToGo: " << _secondsToGo.val << L_endl;
			return &_secondsToGo;
		}
		default: return 0;
		}
	}

	bool Dataset_TowelRail::setNewValue(int fieldID, const I_Data_Formatter * newValue) {
		switch (fieldID) {
		case e_name: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_name = *strWrapper;
			//auto debug = record();
			//debug.rec();
			strcpy(record().rec().name, _name.str());
			setRecordID(record().update());
			break; }
		case e_onTemp:
			_onTemp = *newValue;
			record().rec().onTemp = decltype(record().rec().onTemp)(_onTemp.val);
			setRecordID(record().update());
			break;
		case e_minutesOn: 
			_minutesOn = *newValue;
			record().rec().minutes_on = decltype(record().rec().minutes_on)(_minutesOn.val);
			setRecordID(record().update());
			break;
		}
		return false;
	}
}