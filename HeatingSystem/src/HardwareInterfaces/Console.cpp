#include "Console.h"
#include "I_Keypad.h"
#include "LCD_Display.h"
#include "..\LCD_UI\A_Top_UI.h"
#include "A__Constants.h"
#include <Logging.h>
#include <MemoryFree.h>

extern unsigned long processStart_mS;

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
		//auto freeMem = freeMemory();
		keyPress = _keyPad.getKey();
		//changeInFreeMemory(freeMem, "processKeys getKey");

		//if (keyPress > -1) {
		//	processStart_mS = micros() - processStart_mS;
		//	logger() << "\n** Time to detect Keypress: " << processStart_mS << L_endl;
		//}
		//do { // prevents watchdog timer reset!
			//logger() << "Key val: " << keyPress << L_endl;
			//keyProcessStart = micros();
			//freeMem = freeMemory();
			//changeInFreeMemory(freeMem, "processKeys getmicros");
			doRefresh = true;
			switch (keyPress) {
				// Process Key 400K/100K I2C Clock: takes 0 for time, 47mS for zone temps, 110mS for calendar, 387/1000mS for change zone on Program page
				// Process Key 19mS with RAM-buffered EEPROM.
			case 0:
				logger() << L_time << "\nGotKey Info\n";
				_chapterGenerator.setChapterNo(1 /*Book_Info*/);
				break;
			case 1:
				logger() << L_time << "\nGotKey UP\n";
				_chapterGenerator().rec_up_down(-1);
				//changeInFreeMemory(freeMem, "processKeys rec_up_down");
				break;
			case 2:
				logger() << L_time << "\nGotKey Left\n";
				_chapterGenerator().rec_left_right(-1);
				//changeInFreeMemory(freeMem, "processKeys rec_left_right");
				break;
			case 3:
				logger() << L_time << "\nGotKey Right\n";
				_chapterGenerator().rec_left_right(1);
				//changeInFreeMemory(freeMem, "processKeys rec_left_right");
				break;
			case 4:
				logger() << L_time << "\nGotKey Down\n";
				_chapterGenerator().rec_up_down(1);
				//changeInFreeMemory(freeMem, "processKeys rec_up_down");
				break;
			case 5:
				logger() << L_time << "\nGotKey Back\n";
				_chapterGenerator.backKey();
				//changeInFreeMemory(freeMem, "processKeys rec_prevUI");
				break;
			case 6:
				logger() << L_time << "\nGotKey Select\n";
				_chapterGenerator().rec_select();
				//changeInFreeMemory(freeMem, "processKeys rec_select");
				break;
			case NUM_LOCAL_KEYS + 1:
				// Set backlight to bright.
				_keyPad.wakeDisplay(true);
				//changeInFreeMemory(freeMem, "processKeys wakeDisplay");
				break;
			default:
				doRefresh = _keyPad.isTimeToRefresh(); // true every second
				//changeInFreeMemory(freeMem, "processKeys isTimeToRefresh");
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
				//	logger() << "\tTime to process key mS: " << keyProcessStart/1000 << L_endl;
				//	keyProcessStart = micros();
				//}
				displayIsAwake = _keyPad.wakeDisplay(false);
				//changeInFreeMemory(freeMem, "processKeys wakeDisplay");
#ifndef ZPSIM
				_lcd_UI._lcd->blinkCursor(displayIsAwake);
				//changeInFreeMemory(freeMem, "processKeys blinkCursor");
#endif
				_lcd_UI._lcd->setBackLight(displayIsAwake);
				//changeInFreeMemory(freeMem, "processKeys setBackLight");
				//Edit::checkTimeInEdit(keyPress);
				_chapterGenerator().stream(_lcd_UI); //400K/100K I2C clock. 6/19mS for Clock, 37/120mS Calendar, 44/140mS ZoneTemps, 80/250mS Program.
				//changeInFreeMemory(freeMem, "processKeys stream");
				//if (keyPress > -1) {
				//	keyProcessStart = micros() - keyProcessStart;
				//	//logger() << "\tTime to stream page uS: " << keyProcessStart << L_endl;
				//	keyProcessStart = micros();
				//}
				_lcd_UI._lcd->sendToDisplay(); // 16mS
				//logger() << "sendToDisplay: " << _lcd_UI._lcd->buff() << L_endl;
				//changeInFreeMemory(freeMem, "processKeys sendToDisplay");
				//if (keyPress > -1) {
				//	keyProcessStart = micros() - keyProcessStart;
				//	logger() << "\tTime to sendToDisplay mS: " << keyProcessStart/1000 << L_endl;
				//	//logger() << "\tFree memory: " << freeMemory() << L_endl;
				//}
			}
			keyPress = _keyPad.getKey();
			//changeInFreeMemory(freeMem, "processKeys again-getKey");
		//} while (keyPress != -1);
		return doRefresh;
	}
}

