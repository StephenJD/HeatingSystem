#include "Data_Console.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

	const char RecInt_Console::OLD_D[7] = "OLD-D";
	const char RecInt_Console::_OLD_DK[7] = "OLD-DK";
	const char RecInt_Console::_LCD_D[7] = "LCD-D";
	const char RecInt_Console::_LCD_DK[7] = "LCD-DK";
	const char* RecInt_Console::_options[4] = { OLD_D, _OLD_DK, _LCD_D, _LCD_DK };


	RecInt_Console::RecInt_Console(RemoteKeypad* remoteKeypadArr)
		: _remoteKeypadArr{ remoteKeypadArr }
		, _name("", 6)
		, _console_options(0, ValRange(e_edOneShort, 0, 3), _options)
		{}

	I_Data_Formatter* RecInt_Console::getField(int fieldID) {
		bool canDo = status() == TB_OK;
		switch (fieldID) {
		case e_name:
			if (canDo) {
				//char name[6];
				//strcpy(name, answer().rec().name);
				//strcat(name, "~");
				_name = answer().rec().name;
			}
			return &_name;
		case e_console_options:
			if (canDo) {
				int8_t timeout = answer().rec().timeout;
				_console_options.val = timeout > 3 ? 3 : timeout;
			}
			return &_console_options;
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
		case e_console_options:
			auto id = answer().id() - 1;
			if (id >= 0) {
				auto timeout = newValue->val == 3 ? 30 : newValue->val;
				answer().rec().timeout = timeout;
				answer().update();
				_remoteKeypadArr[id].setWakeTime(timeout);
			}
			break;
		}
		return false;
	}
}