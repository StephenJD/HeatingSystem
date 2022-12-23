#include "Data_Console.h"
#include <Flag_Enum.h>
#include <Logging.h>

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;
	using namespace arduino_logger;

	const char RecInt_Console::_NO_KEYS[5] = "No-K";
	const char RecInt_Console::_KEYS[5] = "Keys";
	const char* RecInt_Console::_options[NO_OF_CONSOLE_MODES] = { _NO_KEYS, _KEYS };


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
				_name = answer().rec().name;
			}
			return &_name;
		case e_console_options:
			if (canDo) {
				auto mode = OLED_Thick_Display::I2C_Flags_Obj{ answer().rec().timeout };
				_console_options.val = mode.is(OLED_Thick_Display::F_ENABLE_KEYBOARD);
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
				auto mode = OLED_Thick_Display::I2C_Flags_Obj{ answer().rec().timeout };
				mode.set(OLED_Thick_Display::F_ENABLE_KEYBOARD, newValue->val)/*.set(OLED_Thick_Display::R_VALIDATE_READ)*/;
				answer().rec().timeout = mode;
				logger() << "New ConsoleMode[" << id << "] :" << mode << L_endl;
				_thickConsole_Arr[id].set_console_mode(mode);
			} else answer().rec().timeout = (answer().rec().timeout & 0x0F) | 0x40;
			answer().update();
			break;
		}
		return false;
	}
}