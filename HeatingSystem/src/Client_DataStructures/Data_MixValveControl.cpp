#include "Data_MixValveControl.h"

namespace arduino_logger {
	Logger& profileLogger();
}
using namespace arduino_logger;

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

	RecInt_MixValveController::RecInt_MixValveController(MixValveController* mixValveArr)
		: _options{ _name0, _name1 }
		, _mixValveArr(mixValveArr)
		, _name_mode(0, ValRange(e_edOneShort, 0, 1), _options)
		, _pos(90, ValRange(e_fixedWidth | e_editAll, -127, 127))
		, _isTemp(90, ValRange(e_fixedWidth | e_editAll, 0, 90))
		, _reqTemp(90, ValRange(e_fixedWidth | e_editAll, 0, 90))
		, _state("", 3)
		{}

	I_Data_Formatter* RecInt_MixValveController::getField(int fieldID) {
		bool canDo = status() == TB_OK;
		switch (fieldID) {
		case e_multiMode:
			if (canDo) {
				strcpy(_name0, answer().rec().name);
				strcpy(_name1, answer().rec().name);
				strcat(_name0, "  ");
				strcat(_name1, "-M");
			}
			_name_mode.val = runTimeData().multi_master_mode();
			return &_name_mode;
		case e_pos:
			if (canDo) {
				if (runTimeData().getReg(Mix_Valve::R_MODE) == Mix_Valve::e_Checking)
					_pos.val = runTimeData().getReg(Mix_Valve::R_VALVE_POS);
				else
					_pos.val = runTimeData().getReg(Mix_Valve::R_COUNT);
				if (_pos.val > 70) _pos.val = 70;
				if (_pos.val < -70) _pos.val = -70;
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
		case e_multiMode:
		{
			runTimeData().enable_multi_master_mode(newValue->val);
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