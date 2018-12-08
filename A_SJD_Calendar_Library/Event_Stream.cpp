#include "Event_Stream.h"
#include "EEPROM.h"
#include "EEPROM_Utilities.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"

using namespace EEPROM_Utilities;
using namespace Utils;

////////////////////////////// Events_Stream /////////////////////////////////////

Events_Stream::Events_Stream(RunT<EpDataT<EVENT_DEF> > & run) : DataStreamT<EpDataT<EVENT_DEF>, RunT<EpDataT<EVENT_DEF> >, IniSet_EpMan, 1>(run), noOfEvents(0) {}

void Events_Stream::newEvent(S1_byte errNo, S1_byte itemNo) {
	// All events reserve errNo for count and errcode
	U2_epAdrr eventAddr = EEPROMtop - noOfEvents * 2;
	U1_byte lastEvent = EEPROM->read(eventAddr);
	if (errNo < 16 && (lastEvent & 0x0F) == errNo) {
		if ((lastEvent>>4) < 15) lastEvent += 16; // upper nibble is count of consecutive same errors
		EEPROM->write(eventAddr, lastEvent);
	} else {
		eventAddr -= 2;
		U2_epAdrr lastTT = f->timeTemps.startAddr() + f->timeTemps.dataSize() * f->numberOf[0].getVal(noAllTTs);
		if (eventAddr > lastTT) {
			EEPROM->write(eventAddr, errNo);
			EEPROM->write(eventAddr+1, itemNo);
			++noOfEvents;
		}
	}
}

char * Events_Stream::getEvent(S1_ind index) {
	return "";
}

void Events_Stream::clearToSerialPort() {
}

const U1_byte Events_Stream::getNoOfStreamItems() const { // returns no of members of the Collection -1
	Serial.print("No of Events:"); Serial.println(noOfEvents);
	return noOfEvents;
}

char * Events_Stream::objectFldStr(S1_ind activeInd, bool inEdit) const {
	U2_epAdrr eventAddr = EEPROMtop - ++activeInd * 2;
	U1_ind errCode = EEPROM->read(eventAddr);
	S1_byte val = EEPROM->read(eventAddr+1);
	U1_count errCount = (errCode >> 4) + 1;
	strcpy(fieldText,cIntToStr(errCount,2,'0'));
	switch (errCode & 0x0F) {
	case ERR_TEMP_SENS_READ:
		strcat(fieldText, "TempSens Fail:");
		strcat (fieldText,cIntToStr(val,2,'0'));
		break;
	case ERR_PORTS_FAILED:
		strcat (fieldText,"Ports Failed");
		break;
	case ERR_READ_VALUE_WRONG:
		strcat (fieldText,"Read wrong on:");
		strcat (fieldText,cIntToStr(val,2,'0'));
		break;
	case ERR_I2C_RESET:
		strcat (fieldText,"Reset");
		break;
	case ERR_I2C_SPEED_FAIL:
		strcat (fieldText,"Spd Tst failed");
		break;
	case ERR_I2C_READ_FAILED:
		strcat (fieldText,"Read Failed:");
		strcat (fieldText,cIntToStr(val,2,'0'));
		break;
	case ERR_I2C_SPEED_RED_FAILED:
		strcat (fieldText,"Spd Redn Fail:");
		strcat (fieldText,cIntToStr(val,2,'0'));
		break;
	case EVT_I2C_SPEED_RED:
		strcat (fieldText,"Reduce Speed:");
		strcat (fieldText,cIntToStr(val,2,'0'));
		break;
	case ERR_MIX_ARD_FAILED:
		strcat (fieldText,"MixV Failed:");
		strcat (fieldText,cIntToStr(val,2,'0'));
		break;
	case ERR_OP_NOT_LAT:
		strcat (fieldText,"OP diff to L:");
		strcat (fieldText,cIntToStr(val,2,'0'));
		break;
	case EVT_THS_COND_CHANGE:
		strcat (fieldText,"ThrSt Cond Red:");
		strcat (fieldText,cIntToStr(val,2,'0'));
		break;
	case EVT_TEMP_REQ_CHANGED:
		strcat (fieldText,"Temp Req Chnge:");
		strcat (fieldText,cIntToStr(val,2,' ',true));
		break;
	case EVT_GAS_TEMP:
		strcat (fieldText,"Gas Temp:");
		strcat (fieldText,cIntToStr(val,2,' ',true));
		break;
	case ERR_RTC_FAILED:
		strcat (fieldText,"Clock:");
		strcat (fieldText,cIntToStr(val,2,' ',true));
		break;
	default:
		strcat (fieldText,"Flow:");
		strcat (fieldText,cIntToStr(errCode,2,' '));
		strcat (fieldText," TmpEr:");
		strcat (fieldText,cIntToStr(val,2,' ',true));
		break;
	}
	fieldCText[0] = 1; //strlen(fieldText); // cursor offset
	fieldCText[1] = 0; // newLine
	return fieldCText;
}