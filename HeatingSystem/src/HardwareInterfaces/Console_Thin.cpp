#include "Console_Thin.h"
#include "LCD_Display.h"
#include "..\LCD_UI\A_Top_UI.h"
#include "A__Constants.h"
#include <Keypad.h>
#include <Logging.h>
#include <MemoryFree.h>

extern unsigned long processStart_mS;
using namespace arduino_logger;

namespace HardwareInterfaces {
	using namespace LCD_UI;

	Console_Thin::Console_Thin(I_Keypad& keyPad, LCD_Display& lcd_display, Chapter_Generator& chapterGenerator) :
		_keyPad(keyPad)
		, _lcd_UI(lcd_display)
		, _chapterGenerator(chapterGenerator) {}

	uint8_t& Console_Thin::consoleMode() { return _keyPad.consoleMode(); }

	bool Console_Thin::processKeys() {
		bool doRefresh;
		//unsigned long keyProcessStart;
		int keyPress; // Time to detect press 1mS
		keyPress = _keyPad.popKey();

		//if (keyPress > -1) {
		//	processStart_mS = micros() - processStart_mS;
		//	logger() << F("\n** Time to detect Keypress: ") << processStart_mS << L_endl;
		//}
			//logger() << F("Key val: ") << keyPress << L_endl;
			//keyProcessStart = micros();
		//logger() << L_time << F("keyPress: ") << keyPress << L_endl;
		doRefresh = true;
		switch (keyPress) {
			// Process Key 400K/100K I2C Clock: takes 0 for time, 47mS for zone temps, 110mS for calendar, 387/1000mS for change zone on Program page
			// Process Key 19mS with RAM-buffered EEPROM.
		case I_Keypad::KEY_INFO:
			logger() << L_time << F("GotKey Info\n");
			_chapterGenerator.setChapterNo(1 /*Book_Info*/);
			break;
		case I_Keypad::KEY_UP:
			logger() << L_time << F("GotKey UP\n");
			_chapterGenerator().rec_up_down(-1);
			break;
		case I_Keypad::KEY_DOWN:
			logger() << L_time << F("GotKey Down\n");
			_chapterGenerator().rec_up_down(1);
			break;
		case I_Keypad::KEY_LEFT:
			logger() << L_time << F("GotKey Left\n");
			_chapterGenerator().rec_left_right(-1);
			break;
		case I_Keypad::KEY_RIGHT:
			logger() << L_time << F("GotKey Right\n");
			_chapterGenerator().rec_left_right(1);
			break;
		case I_Keypad::KEY_BACK:
			logger() << L_time << F("GotKey Back\n");
			_chapterGenerator.backKey();
			break;
		case I_Keypad::KEY_SELECT:
			logger() << L_time << F("GotKey Select\n");
			_chapterGenerator().rec_select();
			break;
		case I_Keypad::KEY_WAKEUP:
			// Set backlight to bright.
			logger() << L_time << F("Wake Key\n");
			_keyPad.setWakeTimer();
			break;
		default:
			doRefresh = _keyPad.oneSecondElapsed();
#if defined (NO_TIMELINE) && defined (ZPSIM)
			{
				static bool test = true;// for testing, prevent refresh after first time through unless key pressed
				doRefresh = test;
				test = false;
			}
#endif
		}
		return doRefresh;
	}

	void Console_Thin::refreshDisplay() { 
		//Serial.println("Stream");
		auto displayIsAwake = _keyPad.displayIsAwake();
#ifndef ZPSIM
		_lcd_UI._lcd->blinkCursor(displayIsAwake);
#endif
		_lcd_UI._lcd->setBackLight(displayIsAwake);
		_chapterGenerator().stream(_lcd_UI); //400K/100K I2C clock. 6/19mS for Clock, 37/120mS Calendar, 44/140mS ZoneTemps, 80/250mS Program.
		_lcd_UI._lcd->sendToDisplay(); // 16mS
		//logger() << L_time << F("SendToDisplay:\t") /*<< _lcd_UI._lcd->buff()*/ << L_endl;
	}
}