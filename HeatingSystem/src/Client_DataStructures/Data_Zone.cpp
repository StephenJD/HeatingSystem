#include "Data_Zone.h"
#include "Conversions.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\HardwareInterfaces\Zone.h"
#include "..\Assembly\Sequencer.h"
#include "Clock.h"

namespace arduino_logger {
	Logger& profileLogger();
}
using namespace arduino_logger;

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;
	using namespace Date_Time;       
	using namespace HardwareInterfaces;
	//***************************************************
	//             Dataset_Zone
	//***************************************************

	RecInt_Zone::RecInt_Zone(Zone* runtimeData)
		: _runTimeData(runtimeData),
		_name("", 6)
		, _abbrev("", 3)
		, _requestTemp(90, ValRange(e_fixedWidth | e_editAll, 10, 90))
		, _isHeating("",2)
		, _autoRatio(0, ValRange(e_fixedWidth | e_editAll, 0, 999))
		, _autoTimeC(0, ValRange(e_fixedWidth | e_editAll, 0, 255))
		, _autoQuality(0, ValRange(e_fixedWidth | e_editAll, 0, 99,0,2))
		, _autoDelay(0, ValRange(e_fixedWidth | e_editAll, -127, 127))
	{
	}

	I_Data_Formatter * RecInt_Zone::getField(int fieldID) {
		//if (recordID() == -1 || status() != TB_OK) return 0;
		bool canDo = status() == TB_OK;
		switch (fieldID) {
		case e_name:
			if (canDo) _name = answer().rec().name;
			return &_name;
		case e_abbrev:
			if (canDo) _abbrev = answer().rec().abbrev;
			return &_abbrev;
		case e_reqTemp:
		case e_remoteReqTemp:
			if (canDo) {
				HardwareInterfaces::Zone& z = runTimeData();
				_requestTemp.val = z.currTempRequest();
				_requestTemp.valRange.maxVal = z.maxUserRequestTemp(fieldID == e_remoteReqTemp);
			}
			return &_requestTemp;
		case e_isTemp:
			if (canDo) {
				HardwareInterfaces::Zone& z = runTimeData();
				_isTemp.val = z.getCurrTemp();
			}
			return &_isTemp;
		case e_isHeating:
			if (canDo) {
				HardwareInterfaces::Zone & z = runTimeData();
				_isHeating = z.isCallingHeat() ? "`!" : "` ";
			}
			return &_isHeating;
		case e_ratio:
			if (canDo) _autoRatio.val = int32_t(answer().rec().autoRatio * 1000. / Zone::RATIO_DIVIDER) ;
			return &_autoRatio;
		case e_timeConst:
			if (canDo) _autoTimeC.val = answer().rec().autoTimeC;
			return &_autoTimeC;
		case e_quality:
			if (canDo) _autoQuality.val = answer().rec().autoQuality;
			return &_autoQuality;
		case e_delay:
			if (canDo) _autoDelay.val = answer().rec().autoDelay;
			return &_autoDelay;
		default: return 0;
		}
	}

	bool RecInt_Zone::setNewValue(int fieldID, const I_Data_Formatter * newValue) {

		switch (fieldID) {
		case e_name: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_name = *strWrapper;
			auto debug = answer();
			debug.rec();
			strcpy(answer().rec().name, _name.str());
			break; }
		case e_abbrev: {
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_abbrev = *strWrapper;
			strcpy(answer().rec().abbrev, _abbrev.str());
			break; }
		case e_remoteReqTemp:
		case e_reqTemp:
		{
			HardwareInterfaces::Zone & z = runTimeData();
			if (z.isDHWzone()) return false;
			auto reqTemp = uint8_t(newValue->val);
			z.changeCurrTempRequest(reqTemp);
			profileLogger() << L_time << answer().rec().name << " Save req temp to: " << reqTemp << " Max: " << _requestTemp.valRange.maxVal << L_endl;
			break;
		}
		case e_ratio:
			_autoRatio = *newValue;
			answer().rec().autoRatio = decltype(answer().rec().autoRatio)(_autoRatio.val * Zone::RATIO_DIVIDER / 1000.);
			break; 
		case e_timeConst: 
			_autoTimeC = *newValue;
			answer().rec().autoTimeC = decltype(answer().rec().autoTimeC)(_autoTimeC.val);
			break; 
		case e_quality:
			_autoQuality = *newValue;
			answer().rec().autoQuality = decltype(answer().rec().autoQuality)(_autoQuality.val);
			break;
		case e_delay:
			_autoDelay = *newValue;
			answer().rec().autoDelay = decltype(answer().rec().autoDelay)(_autoDelay.val);
			break;
		}
		answer().update();
		return false;
	}

	bool RecInt_Zone::actionOn_LR(int fieldID, int moveBy) {
		switch (fieldID) {
		case e_reqTemp: {
			HardwareInterfaces::Zone& z = runTimeData();
			if (moveBy > 0) {
				profileLogger() << answer().rec().name << " Select Next Profile" <<  L_endl;
				//if (z.startDateTime() <= clock_().now()) {
					z.startNextProfile();
				//}
			} else {
				profileLogger() << answer().rec().name << " Restore Current Profile" << L_endl;
				z.revertToScheduledProfile();
			}
			return true;
		}
		default: return false;
		}
	}
}