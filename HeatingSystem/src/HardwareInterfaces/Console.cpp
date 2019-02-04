#include "Console.h"
#include "I_Keypad.h"
#include "LCD_Display.h"
#include "..\LCD_UI\A_Top_UI.h"
#include "A__Constants.h"

namespace HardwareInterfaces {
	using namespace LCD_UI;

	Console::Console(I_Keypad & keyPad, LCD_Display & lcd_display, A_Top_UI & pageGenerator) :
		_keyPad(keyPad)
		, _lcd_UI(lcd_display)
		, _pageGenerator(pageGenerator)
	{}

	bool Console::processKeys() {
		bool doRefresh;
		auto keyPress = _keyPad.getKey();
		do {
			//Serial.print("Main processKeys: "); Serial.println(keyPress);
			doRefresh = true;
			switch (keyPress) {
			case 0:
				//Serial.println("GotKey Info");
				//newPage(mp_infoHome);
				break;
			case 1:
				//Serial.println("GotKey UP");
				_pageGenerator.rec_up_down(1);
				break;
			case 2:
				//Serial.println("GotKey Left");
				_pageGenerator.rec_left_right(-1);
				break;
			case 3:
				//Serial.println("GotKey Right");
				_pageGenerator.rec_left_right(1);
				break;
			case 4:
				//Serial.println("GotKey Down");
				_pageGenerator.rec_up_down(-1);
				break;
			case 5:
				//Serial.println("GotKey Back");
				_pageGenerator.rec_prevUI();
				break;
			case 6:
				//Serial.println("GotKey Select");
				_pageGenerator.rec_select();
				break;
			case NUM_LOCAL_KEYS + 1:
				// Set backlight to bright.
				_keyPad.wakeDisplay(true);
				doRefresh = true;
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
				auto displayIsAwake = _keyPad.wakeDisplay(false);
#ifndef ZPSIM
				_lcd_UI._lcd->blinkCursor(displayIsAwake);
#endif
				_lcd_UI._lcd->setBackLight(displayIsAwake);
				//Edit::checkTimeInEdit(keyPress);
				//U4_byte	lastTick = micros();
				_pageGenerator.stream(_lcd_UI);
				_lcd_UI._lcd->sendToDisplay();
				//Serial.print("Main processKeys: streamToLCD took: "); Serial.println((micros()-lastTick)/1000);
			}
			keyPress = _keyPad.getKey();
		} while (keyPress != -1);
		return doRefresh;
	}
}

