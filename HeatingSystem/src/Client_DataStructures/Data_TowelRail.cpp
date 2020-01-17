#include "Data_TowelRail.h"
#include "..\..\..\Conversions\Conversions.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\HardwareInterfaces\TowelRail.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;
	using namespace Date_Time;       

	namespace { // restrict to local linkage
		TowelRail_Interface towelRail_UI;
	}

	//*************TowelRail_Wrapper****************
	TowelRail_Wrapper::TowelRail_Wrapper(ValRange valRangeArg) : I_UI_Wrapper(0, valRangeArg) {}

	I_Field_Interface & TowelRail_Wrapper::ui() { return towelRail_UI; }

	//************Edit_TowelRail_h***********************

	//************TowelRail_Interface***********************

	const char * TowelRail_Interface::streamData(bool isActiveElement) const {
		const TowelRail_Wrapper * towelRail = static_cast<const TowelRail_Wrapper *>(_wrapper);
		auto dataset_TowelRail = static_cast<Dataset_TowelRail *>(parent()->getData()->data());
		strcpy(scratch, towelRail->name);
		int nameLen = strlen(scratch);
		while (nameLen < sizeof(towelRail->name)) {
			scratch[nameLen] = ' ';
			++nameLen;
		}
		scratch[nameLen] = 0;
		strcat(scratch, "MinsOn:");
		strcat(scratch, intToString(towelRail->minutesOn));

		strcat(scratch, " has:");
		strcat(scratch, intToString(dataset_TowelRail->getField(Dataset_TowelRail::e_secondsToGo)->val));
		return scratch;
	}

	//***************************************************
	//             Dataset_TowelRail
	//***************************************************

	Dataset_TowelRail::Dataset_TowelRail(Query & query, VolatileData * runtimeData, I_Record_Interface * parent)
		: Record_Interface(query, runtimeData, parent),
		_name("", 6),
		_onTemp(90, ValRange(e_editAll, 10, 60)),
		_minutesOn(0, ValRange(e_fixedWidth | e_editAll, 0, 240))
	{}

	I_UI_Wrapper * Dataset_TowelRail::getField(int fieldID) {
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
			return &_secondsToGo;
		//case e_status:

		default: return 0;
		}
	}

	bool Dataset_TowelRail::setNewValue(int fieldID, const I_UI_Wrapper * newValue) {
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
		case e_minutesOn: {
			_minutesOn = *newValue;
			record().rec().minutes_on = decltype(record().rec().minutes_on)(_minutesOn.val);
			setRecordID(record().update());
			break;
		}
		}
		return false;
	}
}