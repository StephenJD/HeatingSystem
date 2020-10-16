#include "Data_Zone.h"
#include "Conversions.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\HardwareInterfaces\Zone.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;
	using namespace Date_Time;       

	namespace { // restrict to local linkage
		ReqIsTemp_Interface reqIsTemp_UI;
	}

	//*************ReqIsTemp_Wrapper****************

	ReqIsTemp_Wrapper::ReqIsTemp_Wrapper(ValRange valRangeArg) : I_Data_Formatter(19, valRangeArg) {}

	I_Streaming_Tool & ReqIsTemp_Wrapper::ui() { return reqIsTemp_UI; }

	//************Edit_ReqIsTemp_h***********************

	int Edit_ReqIsTemp_h::gotFocus(const I_Data_Formatter * data) { // returns initial edit focus
		if (data) currValue() = *data;
		setDecade(1);
		currValue().valRange._cursorPos = 12;
		return currValue().valRange.editablePlaces - 1;
	}

	int Edit_ReqIsTemp_h::cursorFromFocus(int focusIndex) { // Sets CursorPos
		currValue().valRange._cursorPos = 11 + focusIndex;
		setDecade(focusIndex);
		return focusIndex;
	}

	void Edit_ReqIsTemp_h::setInRangeValue() {
		I_Edit_Hndl::setInRangeValue();
		Field_StreamingTool_h & f_int_h = static_cast<Field_StreamingTool_h&>(*backUI());
		f_int_h.backUI()->getItem(f_int_h.backUI()->focusIndex());
		auto wrapper = const_cast<I_Data_Formatter *>(f_int_h.f_interface().getDataFormatter());
		auto req_wrapper = static_cast<ReqIsTemp_Wrapper*>(wrapper);
		auto tempOffset = f_int_h.getData()->data()->getField(Dataset_Zone::e_offset)->val;
		auto & zoneData  = static_cast<Dataset_Zone &>(*f_int_h.getData()->data());
		HardwareInterfaces::Zone & z = zoneData.zone(zoneData.record().id());
		if (tempOffset >= 0) {
			if (currValue().val >= z.maxUserRequestTemp()) currValue().val = z.maxUserRequestTemp();			 
		}
		req_wrapper->val = currValue().val;
		f_int_h.getData()->data()->setNewValue(Dataset_Zone::e_reqTemp, req_wrapper);
	}
	
	//************ReqIsTemp_Interface***********************

	const char * ReqIsTemp_Interface::streamData(bool isActiveElement) const {
		const ReqIsTemp_Wrapper * reqIsTemp = static_cast<const ReqIsTemp_Wrapper *>(_data_formatter);
		int streamVal = getData(isActiveElement);
		strcpy(scratch, reqIsTemp->name);
		int nameLen = strlen(scratch);
		while (nameLen < sizeof(reqIsTemp->name)) {
			scratch[nameLen] = ' ';
			++nameLen;
		}
		scratch[nameLen] = 0;
		strcat(scratch, "Req$");
		strcat(scratch, intToString(streamVal, _data_formatter->valRange.editablePlaces));

		strcat(scratch, " is:");
		strcat(scratch, intToString(reqIsTemp->isTemp));
		strcat(scratch, reqIsTemp->isHeating ? "!" : " ");
		return scratch;
	}

	//***************************************************
	//             Dataset_Zone
	//***************************************************

	Dataset_Zone::Dataset_Zone(Query & query, VolatileData * runtimeData, I_Record_Interface * parent)
		: Record_Interface(query, runtimeData, parent),
		_name("", 6),
		_abbrev("", 3),
		_requestTemp(90, ValRange(e_editAll, 10, 100)),
		_factor(0, ValRange(e_fixedWidth | e_editAll | e_showSign, -100, 100, 1))
		,_reqIsTemp(ValRange(e_fixedWidth | e_editAll,10,90))
	{
	}

	I_Data_Formatter * Dataset_Zone::getField(int fieldID) {
		if (recordID() == -1 || record().status() != TB_OK) return 0;
		switch (fieldID) {
		case e_name:
			_name = record().rec().name;
			return &_name;
		case e_abbrev:
			_abbrev = record().rec().abbrev;
			return &_abbrev;
		case e_reqTemp:
		{
			HardwareInterfaces::Zone & z = zone(record().id());
			_requestTemp.val = z.currTempRequest();
			return &_requestTemp;
		}
		case e_isTemp:
		{
			HardwareInterfaces::Zone & z = zone(record().id());
			_isTemp.val = z.getCurrTemp();
			return &_isTemp;
		}
		case e_factor:
			_factor.val = record().rec().autoTimeC;
			return &_factor;
		case e_reqIsTemp:
		{
			HardwareInterfaces::Zone & z = zone(record().id());
			strcpy(_reqIsTemp.name, record().rec().name);
			_reqIsTemp.isTemp = z.getCurrTemp();
			_reqIsTemp.isHeating = z.isCallingHeat();
			_reqIsTemp.val = z.currTempRequest();
			return &_reqIsTemp;
		}
		case e_offset:
			_tempOffset.val = record().rec().offsetT;
			return &_tempOffset;
		default: return 0;
		}
	}

	bool Dataset_Zone::setNewValue(int fieldID, const I_Data_Formatter * newValue) {

		switch (fieldID) {
		case e_name: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_name = *strWrapper;
			auto debug = record();
			debug.rec();
			strcpy(record().rec().name, _name.str());
			setRecordID(record().update());
			break; }
		case e_abbrev: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_abbrev = *strWrapper;
			strcpy(record().rec().abbrev, _abbrev.str());
			setRecordID(record().update());
			break; }
		case e_reqTemp:
		{
			HardwareInterfaces::Zone & z = zone(record().id());
			z.offsetCurrTempRequest(uint8_t(newValue->val));
			record().rec().offsetT = z.offset();
			setRecordID(record().update());
			break;
		}
		case e_factor: 
			_factor = *newValue;
			record().rec().autoTimeC = decltype(record().rec().autoTimeC)(_factor.val);
			setRecordID(record().update());
			break; 
		case e_reqIsTemp: {
				HardwareInterfaces::Zone & z = zone(record().id());
				z.offsetCurrTempRequest(uint8_t(newValue->val));
				record().rec().offsetT = z.offset();
				setRecordID(record().update());
				break;
			}
		}
		return false;
	}

}