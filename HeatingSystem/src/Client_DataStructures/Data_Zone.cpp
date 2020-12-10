#include "Data_Zone.h"
#include "Conversions.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\HardwareInterfaces\Zone.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;
	using namespace Date_Time;       

	//***************************************************
	//             Dataset_Zone
	//***************************************************

	RecInt_Zone::RecInt_Zone(VolatileData * runtimeData)
		: _runTimeData(runtimeData),
		_name("", 6)
		, _abbrev("", 3)
		, _requestTemp(90, ValRange(e_fixedWidth | e_editAll, 10, 90))
		, _isHeating("",2)
		, _autoRatio(0, ValRange(e_fixedWidth | e_editAll, 0, 255))
		, _autoTimeC(0, ValRange(e_fixedWidth | e_editAll, 0, 255))
		, _autoQuality(0, ValRange(e_editAll, 0, 255))
		, _manHeatTime(0, ValRange(e_fixedWidth | e_editAll, 0, 255))
	{
	}

	HardwareInterfaces::Zone& RecInt_Zone::zone(int index) { return static_cast<HardwareInterfaces::Zone*>(runTimeData())[index]; }

	I_Data_Formatter * RecInt_Zone::getField(int fieldID) {
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
			_requestTemp.valRange.maxVal = z.maxUserRequestTemp();
			return &_requestTemp;
		}
		case e_offset:
			_tempOffset.val = record().rec().offsetT;
			return &_tempOffset;
		case e_isTemp:
		{
			//record().rec(); // trigger answer-load
			HardwareInterfaces::Zone & z = zone(record().id());
			_isTemp.val = z.getCurrTemp();
			return &_isTemp;
		}
		case e_isHeating:
		{
			HardwareInterfaces::Zone & z = zone(record().id());
			_isHeating = z.isCallingHeat() ? "`!" : "` ";
			return &_isHeating;
		}
		case e_ratio:
			_autoRatio.val = record().rec().autoRatio;
			return &_autoRatio;
		case e_timeConst:
			_autoTimeC.val = record().rec().autoTimeC;
			return &_autoTimeC;
		case e_quality:
			_autoQuality.val = record().rec().autoQuality;
			return &_autoQuality;
		case e_minsPerHalfDegree:
			_manHeatTime.val = record().rec().manHeatTime;
			return &_manHeatTime;
		default: return 0;
		}
	}

	bool RecInt_Zone::setNewValue(int fieldID, const I_Data_Formatter * newValue) {

		switch (fieldID) {
		case e_name: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_name = *strWrapper;
			auto debug = record();
			debug.rec();
			strcpy(record().rec().name, _name.str());
			break; }
		case e_abbrev: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_abbrev = *strWrapper;
			strcpy(record().rec().abbrev, _abbrev.str());
			break; }
		case e_reqTemp:
		{
			logger() << "Save req temp on " << record().rec().name << " ID: " << record().id() << L_endl;
			HardwareInterfaces::Zone & z = zone(record().id());
			auto reqTemp = uint8_t(newValue->val);
			z.offsetCurrTempRequest(reqTemp);
			record().rec().offsetT = z.offset();
			break;
		}
		case e_ratio:
			_autoRatio = *newValue;
			record().rec().autoRatio = decltype(record().rec().autoRatio)(_autoRatio.val);
			break; 
		case e_timeConst: 
			_autoTimeC = *newValue;
			record().rec().autoTimeC = decltype(record().rec().autoTimeC)(_autoTimeC.val);
			break; 
		case e_quality:
			_autoQuality = *newValue;
			record().rec().autoQuality = decltype(record().rec().autoQuality)(_autoQuality.val);
			break;
		case e_minsPerHalfDegree:
			_manHeatTime = *newValue;
			record().rec().manHeatTime = decltype(record().rec().manHeatTime)(_manHeatTime.val);
			break;
		}
		record().update();
		return false;
	}

	bool RecInt_Zone::actionOn_LR(int fieldID, int moveBy) {
		switch (fieldID) {
		case e_reqTemp: {
			logger() << "Select Next Sequence Event " << record().rec().name << " ID: " << record().id() << L_endl;
			HardwareInterfaces::Zone& z = zone(record().id());
			if (moveBy > 0 && z.currTempRequest() != z.nextTempRequest()) {
				z.setProfileTempRequest(z.nextTempRequest());
			} else {
				z.
			}
			return true;
		}
		default: return false;
		}
	}

}