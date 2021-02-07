#include "Data_MixValveControl.h"

Logger& profileLogger();

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

	RecInt_MixValveController::RecInt_MixValveController(MixValveController* mixValveArr)
		: _mixValveArr(mixValveArr)
		, _name("", 5)
		, _pos(90, ValRange(e_fixedWidth | e_editAll, -127, 127))
		, _isTemp(90, ValRange(e_fixedWidth | e_editAll, 0, 90))
		, _reqTemp(90, ValRange(e_fixedWidth | e_editAll, 0, 90))
		, _state("", 3)
		{}

	I_Data_Formatter* RecInt_MixValveController::getField(int fieldID) {
		auto mode = runTimeData().readFromValve(Mix_Valve::mode);
		if (recordID() == -1 || status() != TB_OK) return 0;
		switch (fieldID) {
		case e_name:
			_name = answer().rec().name;
			return &_name;
		case e_pos:
			if (mode == Mix_Valve::e_Checking) _pos.val = runTimeData().readFromValve(Mix_Valve::valve_pos);
			else _pos.val = runTimeData().readFromValve(Mix_Valve::count);
			return &_pos;
		case e_flowTemp:
			_isTemp.val = runTimeData().readFromValve(Mix_Valve::flow_temp);
			return &_isTemp;
		case e_reqTemp:
			_reqTemp.val = runTimeData().readFromValve(Mix_Valve::request_temp);
			return &_reqTemp;
		case e_state:
			auto state = runTimeData().readFromValve(Mix_Valve::state);
			if (mode == 0 && state == 0) {
				logger() << "MixValveController read error" << L_endl;
				runTimeData().i2C().extendTimeouts(10000, 10, 5000);
				//auto thisFreq = runTimeData().runSpeed();
				//logger() << F("\t\tslowdown for 0x") << L_hex << runTimeData().getAddress() << F(" speed was ") << L_dec << thisFreq << L_endl;
				//runTimeData().set_runSpeed(max(thisFreq - max(thisFreq / 10, 1000), I2C_Talk::MIN_I2C_FREQ));
				//logger() << F("\t\treduced speed to ") << runTimeData().runSpeed() << L_endl;
			}
			switch (state) {
			case Mix_Valve::e_Moving_Coolest: _state = "Min"; break;
			case Mix_Valve::e_NeedsCooling: _state = "Cl"; break;
			case Mix_Valve::e_NeedsHeating: _state = "Ht"; break;
			case Mix_Valve::e_Off:
				switch (mode) {
				case Mix_Valve::e_Mutex: _state = "Mx"; break;
				case Mix_Valve::e_Wait : _state = "Wt"; break;
				case Mix_Valve::e_Checking: _state = "Ck"; break;
				case Mix_Valve::e_Moving: _state = "Mv"; break;
				default: _state = "MEr";
				}
				break;
			default: _state = "SEr";
			}
			profileLogger() << L_time << L_tabs << _name.str() << _pos.val << _isTemp.val << _reqTemp.val << _state.str() << runTimeData().readFromValve(Mix_Valve::ratio) << L_endl;
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
			runTimeData().setRequestFlowTemp((uint8_t)_reqTemp.val);
			break;
		}
		return false;
	}
}