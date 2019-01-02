#include "Data_Zone.h"
#include "..\..\..\Conversions\Conversions.h"

//#include <iostream>
namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;
	using namespace Date_Time;       

	namespace { // restrict to local linkage
		ReqIsTemp_Interface reqIsTemp_UI;
	}

	//*************ReqIsTemp_Wrapper****************

	ReqIsTemp_Wrapper::ReqIsTemp_Wrapper(ValRange valRangeArg) : I_UI_Wrapper(19, valRangeArg) {}

	I_Field_Interface & ReqIsTemp_Wrapper::ui() { return reqIsTemp_UI; }

	//************Edit_ReqIsTemp_h***********************

	int Edit_ReqIsTemp_h::gotFocus(const I_UI_Wrapper * data) { // returns initial edit focus
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
		Field_Interface_h & f_int_h = static_cast<Field_Interface_h&>(*backUI());
		f_int_h.backUI()->getItem(f_int_h.backUI()->focusIndex());
		auto wrapper = f_int_h.f_interface().getWrapper();
		auto req_wrapper = static_cast<const ReqIsTemp_Wrapper*>(wrapper);
		if (currValue().val > req_wrapper->isTemp + 1) currValue().val = req_wrapper->isTemp + 1;
	}
	
	//************ReqIsTemp_Interface***********************

	const char * ReqIsTemp_Interface::streamData(const Object_Hndl * activeElement) const {
		const ReqIsTemp_Wrapper * reqIsTemp = static_cast<const ReqIsTemp_Wrapper *>(_wrapper);
		int streamVal = getData(activeElement);
		strcpy(scratch, reqIsTemp->name);
		int nameLen = strlen(scratch);
		while (nameLen < sizeof(reqIsTemp->name)) {
			scratch[nameLen] = ' ';
			++nameLen;
		}
		scratch[nameLen] = 0;
		strcat(scratch, "Req$");
		strcat(scratch, intToString(streamVal, _wrapper->valRange.editablePlaces));

		strcat(scratch, " is:");
		strcat(scratch, intToString(reqIsTemp->isTemp));
		strcat(scratch, reqIsTemp->isHeating ? "!" : " ");
		return scratch;
	}

	//***************************************************
	//              Zone Dynamic Class
	//***************************************************

	Zone::Zone(int tempReq, int currTemp)
		: _currProgTempRequest(tempReq),
		_currTempRequest(tempReq),
		_currTemp(currTemp),
		_isHeating(tempReq > currTemp ? true : false)
	{}

	//***************************************************
	//             Dataset_Zone
	//***************************************************

	Dataset_Zone::Dataset_Zone(Query & query, VolatileData * runtimeData, I_Record_Interface * parent)
		: Record_Interface(query, runtimeData, parent),
		_name("", 6),
		_abbrev("", 4),
		_requestTemp(90, ValRange(e_editAll, 10, 100)),
		_factor(0, ValRange(e_fixedWidth | e_editAll | e_showSign, -100, 100, 1))
		,_reqIsTemp(ValRange(e_fixedWidth | e_editAll,10,90))
	{
		//std::cout << "** Zone " << " Addr:" << std::hex << long long(this) << std::endl;
	}

	I_UI_Wrapper * Dataset_Zone::getField(int fieldID) {
		if (recordID() == -1) return 0;
		switch (fieldID) {
		case e_name:
			_name = record().rec().name;
			return &_name;
		case e_abbrev:
			_abbrev = record().rec().abbrev;
			return &_abbrev;
		case e_reqTemp:
			return &_requestTemp;
		case e_factor:
			_factor.val = record().rec().autoTimeC;
			return &_factor;
		case e_reqIsTemp:
		{
			Zone & z = zone(record().id());
			strcpy(_reqIsTemp.name, record().rec().name);
			_reqIsTemp.isTemp = z.getCurrTemp();
			_reqIsTemp.isHeating = z.isCallingHeat();
			_reqIsTemp.val = z.getCurrTempRequest();
			return &_reqIsTemp;
		}
		default: return 0;
		}
	}

	bool Dataset_Zone::setNewValue(int fieldID, const I_UI_Wrapper * newValue) {

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
			_requestTemp = *newValue;
			break; 
		case e_factor: 
			_factor = *newValue;
			record().rec().autoTimeC = decltype(record().rec().autoTimeC)(_factor.val);
			setRecordID(record().update());
			break; 
		case e_reqIsTemp: {
			Zone & z = zone(record().id());
			z.setCurrTempRequest(uint8_t(newValue->val));
			break;
			}
		}
		return false;
	}

}