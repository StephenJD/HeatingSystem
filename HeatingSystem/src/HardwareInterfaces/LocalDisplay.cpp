#include "LocalDisplay.h"
#include <Logging.h>
#include "A__Constants.h"
#include <MemoryFree.h>

using namespace RelationalDatabase;
extern byte BRIGHNESS_PWM;
extern byte CONTRAST_PWM;
extern byte PHOTO_ANALOGUE;

namespace HardwareInterfaces {

	LocalDisplay::LocalDisplay(RelationalDatabase::Query * query) : LCD_Display_Buffer(query), _lcd(localLCDpins().pins)
	{
		_lcd.begin(20, 4);
		setBackLight(true);
		logger() << "\nLocalDisplay Constructed";
	}

	void LocalDisplay::blinkCursor(bool isAwake) {
		if (cursorMode() == e_selected) {
			if (!_wasBlinking) { // show cursor immediatly when selecting page
				//logger() << "page selected\n";
				_wasBlinking = true;
				_cursorOn = false;
			}
			_cursorOn = !_cursorOn;
			if (_cursorOn) _lcd.cursor(); else _lcd.noCursor();
		} else _wasBlinking = false;
	}


	void LocalDisplay::sendToDisplay() {
		//logger() << " *** sendToDisplay() *** " << buff() << L_endl;
		//auto freeMem = freeMemory();
		_lcd.clear();
		//changeInFreeMemory(freeMem, "LocalDisplay::sendToDisplay clear");
		//delay(200);
		//logger() << buff());
		///0////////9/////////9/////////9/////////9/////////9/////////9///////////////////////////////////
		// 03:59:50 pm         Sat 12-Jan-2019     DST Hours: 0        Backlight Contrast
		_lcd.print(buff());
		//changeInFreeMemory(freeMem, "LocalDisplay::sendToDisplay print");
		_lcd.setCursor(cursorCol(), cursorRow());
		//changeInFreeMemory(freeMem, "LocalDisplay::sendToDisplay setCursor");
		switch (cursorMode()) {
		case e_unselected: _lcd.noCursor(); _lcd.noBlink(); break;
		case e_selected:
#ifdef ZPSIM
			_lcd.cursor();
#endif
			_lcd.noBlink();
			break;
		case e_inEdit:  _lcd.noCursor(); _lcd.blink(); break;
		default:;
		}
		//changeInFreeMemory(freeMem, "LocalDisplay::sendToDisplay blink");
	}

	uint8_t LocalDisplay::ambientLight() const { // return 5th root of photo-cell reading * 10. Returns 15 (bright) to 38 (dim).
		uint16_t photo = analogRead(PHOTO_ANALOGUE);
		return uint8_t((1024 - photo) >> 2);
	}

	void LocalDisplay::setBackLight(bool wake) {
		//logger() << "SetBackLight Query set:", (long)_query);
		if (_query) {
			auto displayDataRS = _query->begin();
			//logger() << "\tQuery beginID:", displayDataRS->id());
			Answer_R<R_Display> displayDataA = *_query->begin();
			R_Display displayData = displayDataA.rec();
			uint16_t minBL = displayData.backlight_dim;  // val 0-25
			uint8_t blRange = displayData.backlight_bright - minBL;// val 0-25
			uint8_t minPhoto = displayData.photo_dim; // val 5th root of Dim photo-cell reading * 10, typically = 40
			uint8_t photoRange = displayData.photo_bright;// val 5th root of Bright photo-cell reading * 10, typically = 20
			photoRange = (minPhoto > photoRange) ? minPhoto - photoRange : photoRange - minPhoto;
			//logger() << "SetBackLight... PhotoRange :", photoRange);

			float lightFactor = photoRange > 1 ? float(ambientLight() - minPhoto) / photoRange : 1.f;
			uint16_t brightness = uint16_t(minBL + blRange * lightFactor); // compensate for light
			if (!wake) brightness = (brightness + minBL) / 2; // compensate for sleep
			//logger() << "\t brightness/light :", brightness, lightFactor);
			brightness = (brightness * (MAX_BL - MIN_BL) / 25) + MIN_BL; // convert to analogue write val.
			if (brightness > MAX_BL) brightness = MAX_BL;
			if (brightness < MIN_BL) brightness = MIN_BL;

			//logger() << "\t Brightness :", brightness, "Contrast :", displayData.contrast * 3);
			analogWrite(BRIGHNESS_PWM, brightness); // Useful range is 255 (bright) to 200 (dim)
			analogWrite(CONTRAST_PWM, displayData.contrast * 3); // Useful range is 0 (max) to 70 (min)
		}
	}

	void LocalDisplay::changeContrast(int changeBy) {
		// range 0-25
		if (_query) {
			Answer_R<R_Display> displayData = *_query->begin();
			int contrast = displayData.rec().contrast + changeBy * 4;
			displayData.rec().contrast = uint8_t(contrast);
			displayData.update();
			setBackLight(true);
			logger() << "\n\tChange Contrast : " << displayData.rec().contrast;
		}
	}
	
	void LocalDisplay::changeBacklight(int changeBy) {
		// range 0-25
		if (_query) {
			//logger() << "\nChange Backlight ChangeBy : " << changeBy;
			Answer_R<R_Display> displayData = *_query->begin();
			auto ambient = ambientLight();
			auto midAmbient = (displayData.rec().photo_bright + displayData.rec().photo_dim) / 2;
			//logger() << "\tAmbient :", ambient);
			//logger() << "\tphoto_bright :", displayData.rec().photo_bright);
			//logger() << "\tphoto_dim :", displayData.rec().photo_dim);

			if (ambient > midAmbient) {
				displayData.rec().backlight_bright = uint8_t(displayData.rec().backlight_bright + changeBy);
				displayData.rec().photo_bright = ambient;
				logger() << "\n\tChange Bright Backlight : " << displayData.rec().backlight_bright;
				logger() << "\n\tPhoto Bright : " << displayData.rec().photo_bright;
			}
			else {
				displayData.rec().backlight_dim = uint8_t(displayData.rec().backlight_dim + changeBy);
				uint8_t	dsBrightPhotoVal = displayData.rec().photo_bright;
				//if (ambient > dsBrightPhotoVal) { // swap bright/dim photo vals
				//	logger() << "\tPrevious was dimmer! Swap bright/dim photo vals");
				//	uint8_t temp = ambient;
				//	ambient = dsBrightPhotoVal;
				//	dsBrightPhotoVal = temp;
				//	displayData.rec().photo_bright = dsBrightPhotoVal;
				//}
				displayData.rec().photo_dim = ambient;
				logger() << "\n\tChange Dim Backlight : " << displayData.rec().backlight_dim;
				logger() << "\n\tPhoto Dim : " << displayData.rec().photo_dim;
			}
			displayData.update();
			setBackLight(true);
		}
	}
}