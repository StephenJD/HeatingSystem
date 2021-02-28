#include "Data_MixValveControl.h"

Logger& profileLogger();

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

	RecInt_MixValveController::RecInt_MixValveController(MixValveController* mixValveArr)
		: _mixValveArr(mixValveArr)
		//, _name("", 5)
		, _name("", 3)
		, _wireMode(0, ValRange(e_fixedWidth | e_editAll, 0, 7))
		, _pos(90, ValRange(e_fixedWidth | e_editAll, -127, 127))
		, _isTemp(90, ValRange(e_fixedWidth | e_editAll, 0, 90))
		, _reqTemp(90, ValRange(e_fixedWidth | e_editAll, 0, 90))
		, _state("", 3)
		{}

	I_Data_Formatter* RecInt_MixValveController::getField(int fieldID) {
		auto wireModeStr = [](int wireMode) {
			switch (wireMode) {
			case 0: return "T";
			case 1: return "AT";
			case 2: return "DT";
			case 3: return "ADT";
			case 4: return "TS";
			case 5: return "ATS";
			case 6: return "DTS";
			case 7: return "ADS";
			default: return "";
			}
		};

		//if (recordID() == -1 || status() != TB_OK) return 0;
		bool canDo = status() == TB_OK;
		switch (fieldID) {
		case e_name:
			if (canDo) {
				//_name = answer().rec().name;
				_name = wireModeStr(runTimeData().wireMode);
			}
			return &_name;
		case e_wireMode:
			if (canDo) _wireMode.val = runTimeData().wireMode;
			return &_wireMode;
		case e_pos:
			if (canDo) {
				if (runTimeData().valveStatus.algorithmMode == Mix_Valve::e_Checking)
					_pos.val = runTimeData().valveStatus.valvePos;
				else
					_pos.val = runTimeData().valveStatus.onTime;
				if (_pos.val > 500) _pos.val = 500;
				if (_pos.val < -500) _pos.val = -500;
			}
			return &_pos;
		case e_flowTemp:
			if (canDo) _isTemp.val = runTimeData().flowTemp();
			return &_isTemp;
		case e_reqTemp:
			if (canDo) _reqTemp.val = runTimeData().reqTemp();
			return &_reqTemp;
		case e_state:
			if (canDo) {
				runTimeData().monitorMode();
				_state = (const char*)runTimeData().showState();
			}
			return &_state;
		default: return 0;
		}
	}

	bool RecInt_MixValveController::setNewValue(int fieldID, const I_Data_Formatter* newValue) {
		switch (fieldID) {
		case e_name:
		{
			const StrWrapper* strWrapper(static_cast<const StrWrapper*>(newValue));
			_name = *strWrapper;
			strcpy(answer().rec().name, _name.str());
			answer().update();
			break;
		}
		case e_reqTemp:
			_reqTemp = *newValue;
			runTimeData().sendRequestFlowTemp((uint8_t)_reqTemp.val);
			break;
		case e_wireMode:
			_wireMode = *newValue;
			runTimeData().setWireMode(_wireMode.val);
		}
		return false;
	}
}