#include "Data_TowelRail.h"
#include "..\..\..\Conversions\Conversions.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\HardwareInterfaces\TowelRail.h"

using namespace arduino_logger;

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;
	using namespace Date_Time;       
	using namespace HardwareInterfaces;

	//***************************************************
	//             Dataset_TowelRail
	//***************************************************

	RecInt_TowelRail::RecInt_TowelRail(TowelRail* runtimeData)
		: _runTimeData(runtimeData),
		_name("", 8)
		, _onTemp(90, ValRange(e_editAll, 10, 60))
		, _minutesOn(0, ValRange(e_fixedWidth | e_editAll, 0, 240))
	{}

	I_Data_Formatter * RecInt_TowelRail::getField(int fieldID) {
		bool canDo = status() == TB_OK;
		switch (fieldID) {
		case e_name:
			if (canDo) _name = answer().rec().name;
			return &_name;
		case e_onTemp:
			if (canDo) _onTemp.val = answer().rec().onTemp;
			return &_onTemp;
		case e_minutesOn:
			if (canDo) _minutesOn.val = answer().rec().minutes_on;
			return &_minutesOn;
		case e_secondsToGo:
		{
			if (canDo) {
				runTimeData().check();
				_secondsToGo.val = runTimeData().timeToGo();
				//if (_secondsToGo.val) logger() << L_time << "TowelRail " << recordID() << " timeToGo: " << _secondsToGo.val << L_endl;
			}
			return &_secondsToGo;
		}
		default: return 0;
		}
	}

	bool RecInt_TowelRail::setNewValue(int fieldID, const I_Data_Formatter * newValue) {
		switch (fieldID) {
		case e_name: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_name = *strWrapper;
			//auto debug = record();
			//debug.rec();
			strcpy(answer().rec().name, _name.str());
			answer().update();
			break; }
		case e_onTemp:
			_onTemp = *newValue;
			answer().rec().onTemp = decltype(answer().rec().onTemp)(_onTemp.val);
			answer().update();
			break;
		case e_minutesOn: 
			_minutesOn = *newValue;
			answer().rec().minutes_on = decltype(answer().rec().minutes_on)(_minutesOn.val);
			answer().update();
			break;
		}
		return false;
	}
}