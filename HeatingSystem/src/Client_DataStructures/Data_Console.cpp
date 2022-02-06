#include "Data_Console.h"
#include <Flag_Enum.h>

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

	const char RecInt_Console::_OLS_D[7] = "OLS-D";
	const char RecInt_Console::_OLS_DK[7] = "OLS-DK";
	const char RecInt_Console::_OLM_D[7] = "OLM-D";
	const char RecInt_Console::_OLM_DK[7] = "OLM-DK";
	const char* RecInt_Console::_options[NO_OF_CONSOLE_MODES] = { _OLS_D, _OLS_DK, _OLM_D, _OLM_DK };


	RecInt_Console::RecInt_Console(ConsoleController_Thick* thickConsole_Arr) :
		_thickConsole_Arr{ thickConsole_Arr }
		, _name("", 6)
		, _console_options(0, ValRange(e_edOneShort, 0, NO_OF_CONSOLE_MODES - 1), _options)
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
				uint8_t console_mode = answer().rec().timeout >> 6;
				if (console_mode >= NO_OF_CONSOLE_MODES) console_mode = 1;
				_console_options.val = console_mode;
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
				auto console_mode = static_cast<uint8_t>(newValue->val) << 6;
				auto timeout = answer().rec().timeout;
				timeout = 5;
				timeout |= console_mode;
				answer().rec().timeout = timeout;
				_thickConsole_Arr[id].set_console_mode(timeout);
			} else answer().rec().timeout = (answer().rec().timeout & 0x0F) | 0x40;
			answer().update();
			break;
		}
		return false;
	}
}