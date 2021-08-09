#include "Data_Console.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

	RecInt_Console::RecInt_Console(RemoteKeypad* remoteKeypadArr)
		: _remoteKeypadArr{ remoteKeypadArr }
		, _name("", 5)
		, _keypad_enabled(0, ValRange(e_edOneShort, 0, 1))
		{}

	I_Data_Formatter* RecInt_Console::getField(int fieldID) {
		bool canDo = status() == TB_OK;
		switch (fieldID) {
		case e_name:
			if (canDo) {
				_name = answer().rec().name;
			}
			return &_name;
		case e_keypad_enabled:
			if (canDo) {
				auto timeout = answer().rec().timeout;
				_keypad_enabled.val = timeout ? 1 : 0;
			}
			return &_keypad_enabled;
		default: return 0;
		}
	}

	bool RecInt_Console::setNewValue(int fieldID, const I_Data_Formatter* newValue) {
		switch (fieldID) {
		case e_name:
		{
			const StrWrapper* strWrapper(static_cast<const StrWrapper*>(newValue));
			_name = *strWrapper;
			strcpy(answer().rec().name, _name.str());
			answer().update();
			break;
		}
		case e_keypad_enabled:
			auto id = answer().id() - 1;
			if (id >= 0) {
				auto timeout = newValue->val ? 30 : 0;
				_keypad_enabled.val = timeout ? 1 : 0;
				answer().rec().timeout = timeout;
				answer().update();
				_remoteKeypadArr[id].setWakeTime(timeout);
			}
			break;
		}
		return false;
	}
}