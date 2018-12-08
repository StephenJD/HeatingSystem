#include "Display_Stream.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"
#include "Display_Run.h"
#include "Zone_Stream.h"
#include "Keypad.h"
#include "Relay_Run.h" // for changing mode
#include "B_Displays.h"

using namespace Utils;

#define PHOTO_ANALOGUE 0
#define BRIGHNESS_PWM 5 // pins 5 & 6 are not best for PWM control.
#define CONTRAST_PWM 6

template<>
const char DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT>::setupPrompts[DS_COUNT][13] = {
		"Mode:",
		"Contrast:",
		"Bri't light:",
		"Dim light:",
		"Address:",
		"Zone:"};

template<>
ValRange DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	U1_byte prevVal;

	switch (item) {
	case dsMode: // no special save operation, adjustments are saved as they are made
		switch (myVal) {
		case e_onChange_read_LATafter:
			strcat(fieldText,"Ch L after"); break;
		case e_onChange_read_OUTafter:
			strcat(fieldText,"Ch O after"); break;
		case e_alwaysWrite_read_LATafter:
			strcat(fieldText,"All L aft"); break;
		case e_alwaysCheck_read_OUTafter:
			strcat(fieldText,"All O aft"); break;
		case e_alwaysCheck_read_latBeforeWrite:
			strcat(fieldText,"All L bef/aft"); break;
		case e_alwaysCheck_read_OutBeforeWrite:
			strcat(fieldText,"All O bef/aft"); break;
		}
		Relay_Run::switch_mode = modes(myVal);
		return ValRange(0,0,noOfSwModes-1); // skip setOffsetToEnd for fields with individually edititable characters
	case dsContrast: // no special save operation, adjustments are saved as they are made
		const_cast<DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT>*>(this)->setVal(dsContrast,myVal);
		strcat(fieldText,cIntToStr(myVal,2,'0'));
		return ValRange(2,0,25);
	case dsBrightBacklight:
		prevVal = getVal(dsBrightBacklight);
		const_cast<DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT>*>(this)->setVal(dsBrightBacklight,myVal);
		if (prevVal != myVal) {
			const_cast<DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT>*>(this)->setVal(dsBrightPhoto,Display_Stream::getPhoto());
		}
		strcat(fieldText,cIntToStr(myVal,2,'0'));
		return ValRange(2,0,25);
	case dsDimBacklight:
		{prevVal = getVal(dsDimBacklight);
		U1_byte	dsBrightPhotoVal = getVal(dsBrightPhoto);

		const_cast<DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT>*>(this)->setVal(dsDimBacklight,myVal);
		if (prevVal != myVal) {
			U1_byte dsDimPhotoVal = Display_Stream::getPhoto();
			if (dsDimPhotoVal < dsBrightPhotoVal) {
				U1_byte temp = dsDimPhotoVal;
				dsDimPhotoVal = dsBrightPhotoVal;
				dsBrightPhotoVal = temp;
				const_cast<DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT>*>(this)->setVal(dsBrightPhoto,dsBrightPhotoVal);
			}
			const_cast<DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT>*>(this)->setVal(dsDimPhoto,dsDimPhotoVal);
		}
		}
		strcat(fieldText,cIntToStr(myVal,2,'0'));
		return ValRange(2,0,25);
	case dsAddress: // Address
		strcat(fieldText,cIntToStr(myVal,3,'0'));
		return ValRange(3,0,127); // skip setOffsetToEnd for fields with individually edititable characters
	case dsZone: // Zone
		if (myVal == 255) {
			strcat(fieldText,"Main");
		} else {strcat(fieldText,f->zoneS(myVal).getAbbrev());}
		return ValRange(0,-1,f->zones.last());
	default:
		return ValRange(0);
	}
}

Display_Stream::Display_Stream(Display_Run & run) : DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT>(run), _display(0) {
;
}

void Display_Stream::setDisplay(Displays * display) {
	Display_Stream::_display = display;
}


Display_Stream::~Display_Stream(){
}

void Display_Stream::setBackLight(bool wake) {
	//if (!EEPROMClass::isPresent()) return;
	
	U2_byte minBL = getVal(dsDimBacklight); // val 0-25
	U1_byte blRange = getVal(dsBrightBacklight) - minBL;// val 0-25
	U1_byte minPhoto = getVal(dsDimPhoto); // val 5th root of Dim photo-cell reading * 10, typically = 40
	U1_byte photoRange = getVal(dsBrightPhoto);// val 5th root of Bright photo-cell reading * 10, typically = 20
	//Serial.print("\nminBL:");Serial.println(minBL);
	//Serial.print("blRange:");Serial.println(blRange);
	//Serial.print("minPhoto:");Serial.println(minPhoto);
	//Serial.print("dsBrightPhoto:");Serial.println(photoRange);
	photoRange = (minPhoto > photoRange) ? minPhoto - photoRange : photoRange - minPhoto;
	//Serial.print("photoRange:");Serial.println(photoRange);
	//Serial.print("getPhoto:");Serial.println(getPhoto());

	float lightFactor = photoRange > 1 ? float(minPhoto - getPhoto()) / photoRange : 1.;
	U2_byte brightness = U2_byte(minBL + blRange * lightFactor); // compensate for light
	if (!wake) brightness = (brightness + minBL)/2; // compensate for sleep
	brightness = (brightness * (MAX_BL - MIN_BL)/25) + MIN_BL; // convert to analogue write val.
	if (brightness > MAX_BL) brightness = MAX_BL;
	if (brightness < MIN_BL) brightness = MIN_BL;

	analogWrite(BRIGHNESS_PWM,brightness); // Useful range is 255 (bright) to 200 (dim)
	analogWrite(CONTRAST_PWM,epD().getVal(dsContrast) * 3); // Useful range is 0 (max) to 70 (min)
	//logToSD("setBackLight brightness:", brightness, "Contrast:",epD().getVal(dsContrast) * 3);
}

float Display_Stream::getPhoto() { // return 5th root of photo-cell reading * 10. Returns 15 (bright) to 38 (dim).
	U2_byte photo = analogRead(PHOTO_ANALOGUE);
	float result = 1.1F;
	while (result * result * result * result * result < photo) {
		result += 0.1F;
	}
	return result * 10;
}