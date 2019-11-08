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

	Console::Console(I_Keypad & keyPad, LCD_Display & lcd_display, A_Top_UI & pageGenerator) :
		_keyPad(keyPad)
		, _lcd_UI(lcd_display)
		, _pageGenerator(pageGenerator)
	{}

	bool Console::processKeys() {
		bool doRefresh;
		bool displayIsAwake;
		unsigned long keyProcessStart;
		int keyPress; // Time to detect press 1mS
		auto freeMem = freeMemory();
		keyPress = _keyPad.getKey();
		changeInFreeMemory(freeMem, "processKeys getKey");

		//if (keyPress > -1) {
		//	processStart_mS = micros() - processStart_mS;
		//	logger() << "\n** Time to detect Keypress: " << processStart_mS << L_endl;
		//}
		do {
			//logger() << "Key val: " << keyPress << L_endl;
			keyProcessStart = micros();
			changeInFreeMemory(freeMem, "processKeys getmicrosKey");
			doRefresh = true;
			switch (keyPress) {
				// Process Key 400K/100K I2C Clock: takes 0 for time, 47 for zone temps, 110 for calendar, 387/1000mS for change zone on Program page
			case 0:
				//Serial.println("GotKey Info");
				//newPage(mp_infoHome);
				break;
			case 1:
				logger() << "GotKey UP\n";
				_pageGenerator.rec_up_down(-1);
				changeInFreeMemory(freeMem, "processKeys rec_up_down");
				break;
			case 2:
				logger() << "GotKey Left\n";
				_pageGenerator.rec_left_right(-1);
				changeInFreeMemory(freeMem, "processKeys rec_left_right");
				break;
			case 3:
				logger() << "GotKey Right\n";
				_pageGenerator.rec_left_right(1);
				changeInFreeMemory(freeMem, "processKeys rec_left_right");
				break;
			case 4:
				logger() << "GotKey Down\n";
				_pageGenerator.rec_up_down(1);
				changeInFreeMemory(freeMem, "processKeys rec_up_down");
				break;
			case 5:
				logger() << "GotKey Back\n";
				_pageGenerator.rec_prevUI();
				changeInFreeMemory(freeMem, "processKeys rec_prevUI");
				break;
			case 6:
				logger() << "GotKey Select\n";
				_pageGenerator.rec_select();
				changeInFreeMemory(freeMem, "processKeys rec_select");
				break;
			case NUM_LOCAL_KEYS + 1:
				// Set backlight to bright.
				_keyPad.wakeDisplay(true);
				changeInFreeMemory(freeMem, "processKeys wakeDisplay");
				break;
			default:
				doRefresh = _keyPad.isTimeToRefresh(); // true every second
				changeInFreeMemory(freeMem, "processKeys isTimeToRefresh");
#if defined (NO_TIMELINE) && defined (ZPSIM)
				{static bool test = true;// for testing, prevent refresh after first time through unless key pressed
				doRefresh = test;
				test = false;
				}
#endif
			}
			if (doRefresh) {
				if (keyPress > -1) {
					keyProcessStart = micros() - keyProcessStart;
					logger() << "\tTime to process key: " << keyProcessStart << L_endl;
					keyProcessStart = micros();
				}
				displayIsAwake = _keyPad.wakeDisplay(false);
				changeInFreeMemory(freeMem, "processKeys wakeDisplay");
#ifndef ZPSIM
				_lcd_UI._lcd->blinkCursor(displayIsAwake);
				changeInFreeMemory(freeMem, "processKeys blinkCursor");
#endif
				_lcd_UI._lcd->setBackLight(displayIsAwake);
				changeInFreeMemory(freeMem, "processKeys setBackLight");
				//Edit::checkTimeInEdit(keyPress);
				_pageGenerator.stream(_lcd_UI); //400K/100K I2C clock. 6/19mS for Clock, 37/120mS Calendar, 44/140mS ZoneTemps, 80/250mS Program.
				changeInFreeMemory(freeMem, "processKeys stream");
				if (keyPress > -1) {
					keyProcessStart = micros() - keyProcessStart;
					logger() << "\tTime to stream page: " << keyProcessStart << L_endl;
					keyProcessStart = micros();
				}
				_lcd_UI._lcd->sendToDisplay(); // 16mS
				//logger() << "sendToDisplay: " << _lcd_UI._lcd->buff() << L_endl;
				changeInFreeMemory(freeMem, "processKeys sendToDisplay");
				if (keyPress > -1) {
					keyProcessStart = micros() - keyProcessStart;
					logger() << "\tTime to sendToDisplay: " << keyProcessStart << L_endl;
					//logger() << "\tFree memory: " << freeMemory() << L_endl;
				}
			}
			keyPress = _keyPad.getKey();
			//changeInFreeMemory(freeMem, "processKeys again-getKey");
		} while (keyPress != -1);
		return doRefresh;
	}
}

