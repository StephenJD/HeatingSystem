#include "LocalDisplay.h"
#include <Logging/Logging.h>
#include "A__Constants.h"

using namespace RelationalDatabase;

namespace HardwareInterfaces {

	LocalDisplay::LocalDisplay(RelationalDatabase::Query * query) : LCD_Display_Buffer(query), _lcd(localLCDpins().pins)
	{
		_lcd.begin(20, 4);
		_lcd.clear();
		print("Heating Controller");
		logger().log("LocalDisplay Constructed");
	}

	void LocalDisplay::sendToDisplay() {
		_lcd.clear();
		_lcd.print(buff());
		_lcd.setCursor(cursorCol(), cursorRow());
		switch (cursorMode()) {
		case e_unselected: _lcd.noCursor(); _lcd.noBlink(); break;
		case e_selected: _lcd.cursor(); _lcd.noBlink(); break;
		case e_inEdit:  _lcd.noCursor(); _lcd.blink(); break;
		default:;
		}
	}

	uint8_t LocalDisplay::ambientLight() const { // return 5th root of photo-cell reading * 10. Returns 15 (bright) to 38 (dim).
		uint16_t photo = analogRead(PHOTO_ANALOGUE);
		float result = 1.1F;
		while (result * result * result * result * result < photo) {
			result += 0.1F;
		}
		return uint8_t(result * 10);
	}

	void LocalDisplay::setBackLight(bool wake) {
		if (_query) {
			Answer_R<R_Display> displayDataA = *_query->begin();
			R_Display displayData = displayDataA.rec();
			//if (!EEPROMClass::isPresent()) return;
			uint16_t minBL = displayData.backlight_dim;  // val 0-25
			uint8_t blRange = displayData.backlight_bright - minBL;// val 0-25
			uint8_t minPhoto = displayData.photo_dim; // val 5th root of Dim photo-cell reading * 10, typically = 40
			uint8_t photoRange = displayData.photo_bright;// val 5th root of Bright photo-cell reading * 10, typically = 20
			//Serial.print("\nminBL:");Serial.println(minBL);
			//Serial.print("blRange:");Serial.println(blRange);
			//Serial.print("minPhoto:");Serial.println(minPhoto);
			//Serial.print("dsBrightPhoto:");Serial.println(photoRange);
			photoRange = (minPhoto > photoRange) ? minPhoto - photoRange : photoRange - minPhoto;
			//Serial.print("photoRange:");Serial.println(photoRange);
			//Serial.print("ambientLight:");Serial.println(ambientLight());

			float lightFactor = photoRange > 1 ? float(minPhoto - ambientLight()) / photoRange : 1.f;
			uint16_t brightness = uint16_t(minBL + blRange * lightFactor); // compensate for light
			if (!wake) brightness = (brightness + minBL) / 2; // compensate for sleep
			brightness = (brightness * (MAX_BL - MIN_BL) / 25) + MIN_BL; // convert to analogue write val.
			if (brightness > MAX_BL) brightness = MAX_BL;
			if (brightness < MIN_BL) brightness = MIN_BL;

			analogWrite(BRIGHNESS_PWM, brightness); // Useful range is 255 (bright) to 200 (dim)
			analogWrite(CONTRAST_PWM, displayData.contrast * 3); // Useful range is 0 (max) to 70 (min)
			//logToSD("setBackLight brightness:", brightness, "Contrast:",epD().getVal(dsContrast) * 3);
		}
	}

	void LocalDisplay::saveContrast(int contrast) {
		// range 0-25
		if (_query) {
			Answer_R<R_Display> displayData = *_query->begin();
			displayData.rec().contrast = contrast;
			displayData.update();
		}
	}

	void LocalDisplay::saveBrightBacklight(int backlight) {
		// range 0-25
		if (_query) {
			Answer_R<R_Display> displayData = *_query->begin();
			uint8_t	prevVal = displayData.rec().backlight_bright;
			if (prevVal != backlight) {
				displayData.rec().backlight_bright = backlight;
				displayData.rec().photo_bright = ambientLight();
				displayData.update();
			}
		}
	}

	void LocalDisplay::saveDimBacklight(int backlight) {
		// range 0-25
		if (_query) {
			Answer_R<R_Display> displayData = *_query->begin();
			uint8_t	prevVal = displayData.rec().backlight_dim;
			if (prevVal != backlight) {
				displayData.rec().backlight_dim = backlight;
				uint8_t dsDimPhotoVal = ambientLight();
				uint8_t	dsBrightPhotoVal = displayData.rec().photo_bright;
				if (dsDimPhotoVal < dsBrightPhotoVal) { // swap bright/dim photo vals
					uint8_t temp = dsDimPhotoVal;
					dsDimPhotoVal = dsBrightPhotoVal;
					dsBrightPhotoVal = temp;
					displayData.rec().photo_bright = dsBrightPhotoVal;
				}
				displayData.rec().photo_dim = dsDimPhotoVal;
			}
			displayData.update();
		}
	}
}