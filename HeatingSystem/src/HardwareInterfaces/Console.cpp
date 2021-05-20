#include "Console.h"
#include <LCD_Display.h>
#include "..\LCD_UI\A_Top_UI.h"
#include "A__Constants.h"
#include <Keypad.h>
#include <Logging.h>
#include <MemoryFree.h>
#include "I2C_Comms.h"

using namespace arduino_logger;
extern unsigned long processStart_mS;
void ui_yield();

namespace HardwareInterfaces {
	using namespace LCD_UI;

	Console::Console(I_Keypad & keyPad, LCD_Display & lcd_display, Chapter_Generator & chapterGenerator) :
		_keyPad(keyPad)
		, _lcd_UI(lcd_display)
		, _chapterGenerator(chapterGenerator)
	{}

	bool Console::processKeys() {
		bool doRefresh;
		bool displayIsAwake;
		//unsigned long keyProcessStart;
		int keyPress; // Time to detect press 1mS
		keyPress = _keyPad.getKey();

		//if (keyPress > -1) {
		//	processStart_mS = micros() - processStart_mS;
		//	logger() << F("\n** Time to detect Keypress: ") << processStart_mS << L_endl;
		//}
		do { // prevents watchdog timer reset!
			//logger() << F("Key val: ") << keyPress << L_endl;
			//keyProcessStart = micros();
			doRefresh = true;
			switch (keyPress) {
				// Process Key 400K/100K I2C Clock: takes 0 for time, 47mS for zone temps, 110mS for calendar, 387/1000mS for change zone on Program page
				// Process Key 19mS with RAM-buffered EEPROM.
			case 0:
				//logger() << L_time << F("GotKey Info\n");
				_chapterGenerator.setChapterNo(1 /*Book_Info*/);
				break;
			case 1:
				//logger() << L_time << F("GotKey UP\n");
				_chapterGenerator().rec_up_down(-1);
				break;
			case 2:
				//logger() << L_time << F("GotKey Left\n");
				_chapterGenerator().rec_left_right(-1);
				break;
			case 3:
				//logger() << L_time << F("GotKey Right\n");
				_chapterGenerator().rec_left_right(1);
				break;
			case 4:
				//logger() << L_time << F("GotKey Down\n");
				_chapterGenerator().rec_up_down(1);
				break;
			case 5:
				//logger() << L_time << F("GotKey Back\n");
				_chapterGenerator.backKey();
				break;
			case 6:
				//logger() << L_time << F("GotKey Select\n");
				_chapterGenerator().rec_select();
				break;
			case 7:
				// Set backlight to bright.
				_keyPad.wakeDisplay(true);
				break;
			default:
				doRefresh = _keyPad.isTimeToRefresh(); // true every second
#if defined (NO_TIMELINE) && defined (ZPSIM)
				{static bool test = true;// for testing, prevent refresh after first time through unless key pressed
				doRefresh = test;
				test = false;
				}
#endif
			}
			if (doRefresh) {
				//if (keyPress > -1) {
				//	keyProcessStart = micros() - keyProcessStart;
				//	logger() << F("\tTime to process key mS: ") << keyProcessStart/1000 << L_endl;
				//	keyProcessStart = micros();
				//}
				displayIsAwake = _keyPad.wakeDisplay(false);
				//if (!displayIsAwake) { // move off start-page
					//if (_chapterGenerator.chapter() == 0 && _chapterGenerator.page() == 0) _chapterGenerator().rec_up_down(1);
				//}
#ifndef ZPSIM
				_lcd_UI._lcd->blinkCursor(displayIsAwake);
				//changeInFreeMemory(freeMem, "processKeys blinkCursor");
#endif
				_lcd_UI._lcd->setBackLight(displayIsAwake);
				//Edit::checkTimeInEdit(keyPress);
				_chapterGenerator().stream(_lcd_UI); //400K/100K I2C clock. 6/19mS for Clock, 37/120mS Calendar, 44/140mS ZoneTemps, 80/250mS Program.
				//if (keyPress > -1) {
				//	keyProcessStart = micros() - keyProcessStart;
				//	//logger() << F("\tTime to stream page uS: ") << keyProcessStart << L_endl;
				//	keyProcessStart = micros();
				//}
				_lcd_UI._lcd->sendToDisplay(); // 16mS
				//logger() << F("sendToDisplay: ") << _lcd_UI._lcd->buff() << L_endl;
				//if (keyPress > -1) {
				//	keyProcessStart = micros() - keyProcessStart;
				//	logger() << F("\tTime to sendToDisplay mS: ") << keyProcessStart/1000 << L_endl;
				//}
			}
			keyPress = _keyPad.getKey();
			//ui_yield();
		} while (keyPress != -1);
		return doRefresh;
	}
}

