#include "Data_MixValveControl.h"

namespace arduino_logger {
	Logger& profileLogger();
}
using namespace arduino_logger;

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

	RecInt_MixValveController::RecInt_MixValveController(MixValveController* mixValveArr)
		: _mixValveArr(mixValveArr)
		, _name("", 5)
		, _pos(0, ValRange(e_fixedWidth | e_editAll, 0, 255))
		, _isTemp(55, ValRange(e_fixedWidth | e_editAll, 0, 90))
		, _reqTemp(55, ValRange(e_fixedWidth | e_editAll, 0, 90))
		, _state("", 3)
		{}

	I_Data_Formatter* RecInt_MixValveController::getField(int fieldID) {
		bool canDo = status() == TB_OK;
		switch (fieldID) {
		case e_name:
			if (canDo) {
				_name = answer().rec().name;
			}
			return &_name;
		case e_pos:
			if (canDo) {
				auto reg = runTimeData().registers();
				auto mode = reg.get(Mix_Valve::R_MODE);
				if (mode == Mix_Valve::e_NewReq || mode == Mix_Valve::e_Wait)
					_pos.val = reg.get(Mix_Valve::R_COUNT);
				else
					_pos.val = reg.get(Mix_Valve::R_VALVE_POS);
			}
			return &_pos;
		case e_flowTemp:
			if (canDo) _isTemp.val = runTimeData().flowTemp();
			return &_isTemp;
		case e_reqTemp:
			if (canDo) _reqTemp.val = runTimeData().reqTemp();
			return &_reqTemp;
		case e_state:
			if (canDo && runTimeData().isEnabled()) {
				runTimeData().monitorMode();
				_state = (const char*)runTimeData().showState(0);
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
		}
		return false;
	}

}