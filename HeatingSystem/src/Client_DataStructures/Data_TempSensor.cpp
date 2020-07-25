#include "Data_TempSensor.h"
#include "Conversions.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;

	namespace { // restrict to local linkage
		TempSensor_UIinterface tempSensor_UI;
	}

	//************* TempSensor_Wrapper ****************

	TempSensor_Wrapper::TempSensor_Wrapper(ValRange valRangeArg) : I_UI_Wrapper(0, valRangeArg) {}

	I_Field_Interface & TempSensor_Wrapper::ui() { return tempSensor_UI; }

	////************ Edit_TempSensor_h ***********************
	//
	//int Edit_TempSensor_h::gotFocus(const I_UI_Wrapper * data) { // returns initial edit focus
	//	if (data) currValue() = *data;
	//	//setDecade(1);
	//	//currValue().valRange._cursorPos = 12;
	//	return currValue().valRange.editablePlaces; // -1;
	//}

	//int Edit_TempSensor_h::cursorFromFocus(int focusIndex) { // Sets CursorPos
	//	//currValue().valRange._cursorPos = 11 + focusIndex;
	//	//setDecade(focusIndex);
	//	return focusIndex;
	//}

	//void Edit_TempSensor_h::setInRangeValue() {
	//	I_Edit_Hndl::setInRangeValue();
	//	Field_Interface_h & f_int_h = static_cast<Field_Interface_h&>(*backUI());
	//	f_int_h.backUI()->getItem(f_int_h.backUI()->focusIndex());
	//	//auto wrapper = f_int_h.f_interface().getWrapper();
	//}

	//***************************************************
	//              TempSensor_UIinterface
	//***************************************************

	const char * TempSensor_UIinterface::streamData(bool isActiveElement) const {
		const TempSensor_Wrapper * tempSensor = static_cast<const TempSensor_Wrapper *>(_wrapper);
		strcpy(scratch, tempSensor->name);
		int nameLen = strlen(scratch);
		while (nameLen < sizeof(tempSensor->name)) {
			scratch[nameLen] = ' ';
			++nameLen;
		}
		scratch[nameLen] = 0;
		strcat(scratch, ":");
		if (tempSensor->temperature == -127) strcat(scratch, "Err");
		else strcat(scratch, intToString(tempSensor->temperature));
		return scratch;
	}

	//***************************************************
	//              TempSensor Dynamic Class
	//***************************************************

	//***************************************************
	//              Dataset_TempSensor
	//***************************************************

	Dataset_TempSensor::Dataset_TempSensor(Query & query, VolatileData * runtimeData, I_Record_Interface * parent)
		: Record_Interface(query, runtimeData, parent)
		, _tempSensor(ValRange(e_fixedWidth, 0, 90))
	{
	}

	I_UI_Wrapper * Dataset_TempSensor::getField(int fieldID) {
		if (recordID() == -1 || record().status() != TB_OK) return 0;
		switch (fieldID) {
		case e_temp:
		{
			HardwareInterfaces::UI_TempSensor & ts = tempSensor(record().id());
			if (ts.readTemperature() != I2C_Talk_ErrorCodes::_OK ) _tempSensor.temperature = -127;
			else _tempSensor.temperature = ts.get_temp();
			return &_tempSensor;
		}		
		case e_name_temp:
		{
			HardwareInterfaces::UI_TempSensor & ts = tempSensor(record().id());
			strcpy(_tempSensor.name, record().rec().name);
			if (ts.readTemperature() != I2C_Talk_ErrorCodes::_OK ) _tempSensor.temperature = -127;
			else _tempSensor.temperature = ts.get_temp();
			return &_tempSensor;
		}
		default: return 0;
		}
	}
}