#include "debgSwitch.h"
#include "C_Displays.h"
#include "D_Factory.h"
#include "MultiCrystal.h"
#include "I2C_Helper.h"
#include "Zone_Run.h"
#include "Zone_Stream.h"
//bool i2c_slowdown_and_reset(I2C_Helper & i2c, int);

extern MultiCrystal * mainLCD;
extern I2C_Helper_Auto_Speed<27> * i2C;
//////////////////////////////////////

#define REG_IOCON_A 0x0A
#define REG_IOCON_B 0x0B
#define REG_IODIR 0x00
#define REG_GPPU 0x0C
#define REG_GPINTEN 0x0B
#define REG_DEFVAL 0x0B
#define REG_INTCON 0x0B
#define REG_INTF 0x0B
#define REG_INTCAP 0x0B
#define REG_GPIO 0x0B

// ********************* Main_display *******************
// creates an array of all pages for the main Displays

//enum E_Main_Pages {mp_start,mp_sel_setup,mp_set_displ,
//	mp_set_nosOf,mp_set_thmStore,mp_set_mixVlves,mp_set_relays,mp_set_tempSnsrs,mp_set_zones,mp_set_twlRads,mp_set_occHtrs,
//	mp_zoneUsrSet,MP_NO_OF_MAIN_PAGES}  **************  defined in EpData ****************

uint8_t resetI2C(I2C_Helper & i2c, int addr);

Main_display::Main_display(MultiCrystal * lcd, Keypad * keypad) : Displays(lcd, keypad) {
	logToSD("Main_display Constructor");
	set_noOf(MP_NO_OF_MAIN_PAGES);
	//mainLCD->createChar(1, UpDnChar);
	//mainLCD->createChar(2, flameChar);
	//mainLCD->createChar(3, cmdChar);
}

Collection & Main_display::get_member_at(S1_ind index) const {
	if (pageIndex != index) {
		pageIndex = index;
		delete page;
		Main_display * thisPtr = const_cast<Main_display *>(this);
		switch (index) {
		case mp_start:			page = new Start_Page(thisPtr); break;
		case mp_sel_setup:		page = new Name_List_Pages(thisPtr, f->iniSetup, "Select Setup Screen", false);break;
		case mp_set_displ:		page = new Name_List_Pages(thisPtr,f->displays, "Display$ ");break;
		case mp_set_nosOf:		page = new Name_List_Pages(thisPtr,f->numberOf, "Number Of:",false);break;
		case mp_set_thmStore:	page = new Name_List_Pages(thisPtr,f->thermStore, "Thermal Store:",false);break;
		case mp_set_mixVlves:	page = new Name_List_Pages(thisPtr,f->mixingValves, "MixValve$ ");break;
		case mp_set_relays:		page = new Name_List_Pages(thisPtr,f->relays, "Relay$ ");break;
		case mp_set_tempSnsrs:	page = new Object_List_Pages(thisPtr,f->tempSensors, "Temp Sens. $:Addr");break;
		case mp_set_zones:		page = new Name_List_Pages(thisPtr,f->zones, "Zone$ ");break;
		case mp_set_twlRads:	page = new Name_List_Pages(thisPtr,f->towelRads, "TwlRad$ ");break;
		case mp_set_occHtrs:	page = new Name_List_Pages(thisPtr,f->occasionalHeaters, "OclHtr$ ");break;
		case mp_zoneUsrSet:		page = new Zone_UsrSet_Page(thisPtr);break;
		case mp_currDTime:		page = new Cur_DateTime_Page(thisPtr);break;
		case mp_ZoneTemps:		page = new Object_List_Pages(thisPtr,f->zones, "");break;
		case mp_diary:			page = new Diary_Page(thisPtr);break;
		case mp_edit_diary:		page = new EditDiary_Page(thisPtr);break;
		case mp_prog:			page = new Program_Page(thisPtr);break;
		case mp_infoHome:		page = new Name_List_Pages(thisPtr,f->infoMenu, "Select Info:", false);break;
		case mp_infoStore:		page = new Info_List_Pages(thisPtr,f->thermStore, "Thm-Store: ");break;
		case mp_infoZones:		page = new Object_List_Pages(thisPtr,f->zones, "Zone: ");break;
		case mp_infoMixValves:	page = new Object_List_Pages(thisPtr,f->mixingValves, "MixValve: ");break;
		case mp_infoTowelRails:	page = new Object_List_Pages(thisPtr,f->towelRads, "TowelRad: ");break;
		case mp_infoEvents:		page = new Info_List_Pages(thisPtr,f->events, "Logs: ");break;
		case mp_infoTest:		page = new Test_I2C_Page(thisPtr);break;
		}
	}
	return * page;
}

#if defined (ZPSIM)
void Main_display::setKey(S1_byte key) {
	keypad->simKey = key;
	localKeyboardInterrupt();
}
#endif

bool Main_display::processKeys(){
	bool doRefresh; 
	S1_byte keyPress = keypad->getKey();
	do  {
		//Serial.print("Main processKeys: "); Serial.println(keyPress);
		doRefresh = true;
		switch (keyPress) {
			case 0:
				//Serial.println("GotKey Info");
				newPage(mp_infoHome);
				break;
			case 1:
				//Serial.println("GotKey UP");
				up_down_key(1);
				break;
			case 2:
				//Serial.println("GotKey Left");
				left_right_key(-1);
				break;         
			case 3:
				//Serial.println("GotKey Right");
				left_right_key(1);
				break;
			case 4:
				//Serial.println("GotKey Down");
				up_down_key(-1);
				break;
			case 5:
				//Serial.println("GotKey Back");
				back_key();
				break;
			case 6:
				//Serial.println("GotKey Select");
				select_key();
				break;
			default:
				doRefresh = keypad->isTimeToRefresh();
				#if defined (NO_TIMELINE) && defined (ZPSIM)
					{static bool test = true;// for testing, prevent refresh after first time through unless key pressed
					doRefresh = test;
					test = false;
					}
				#endif
		}
		if (doRefresh) {
			Edit::checkTimeInEdit(keyPress);
			//U4_byte	lastTick = micros();
			streamToLCD(true);
			//Serial.print("Main processKeys: streamToLCD took: "); Serial.println((micros()-lastTick)/1000);
		}
		keyPress = keypad->getKey();
	} while (keyPress != -1);
	return doRefresh;
}

// Backlight on analogueWrite pin 6,
// Light sensor on analogue 1.


////////////////////// Remote Display ///////////////////////
Remote_display::Remote_display(MultiCrystal * lcd, Keypad * keypad, U1_ind zone) : Displays(lcd, keypad), _zone(zone) {
	set_noOf(RP_NO_OF_REMOTE_PAGES);
}

Collection & Remote_display::get_member_at(S1_ind index) const {
	if (pageIndex != index) {
		pageIndex = index;
		delete page;
		Remote_display * thisPtr = const_cast<Remote_display *>(this);
		switch (index) {
		case rp_ZoneTemp:		page = new Remote_Temp_Page(thisPtr,f->zones, "^v adjusts temp");break;
		//case mp_diary:			page = new Diary_Page(thisPtr);break;
		//case mp_edit_diary:		page = new EditDiary_Page(thisPtr);break;
		//case mp_prog:			page = new Program_Page(thisPtr);break;
		}
	}
	return * page;
}

#if defined (ZPSIM)
void Remote_display::setKey(S1_byte key) {
	keypad->simKey = key;
	keypad->readKey();
}
#endif

bool Remote_display::processKeys() {
	//if (i2C->notExists(lcd()->i2cAddress())) {
	//	if (processReset) logToSD("Remote Display failed : ",lcd()->i2cAddress());
	//	resetI2C(*i2C,lcd()->i2cAddress());
	//	return false;
	//}
	bool doRefresh;
	char lcdName[3];
	switch (lcd()->i2cAddress()) {
		case 0x24: strcpy(lcdName,"US"); break;
		case 0x25: strcpy(lcdName,"Fl"); break;
		case 0x26: strcpy(lcdName,"DS"); break;
		default: strcpy(lcdName,"??"); break;
	}

	S1_byte keyPress = keypad->getKey();
	do  {
		//Serial.print("Remote processKeys: "); Serial.println(keyPress);
		doRefresh = true;
		switch (keyPress) {
			case 1:
				logToSD("Got Remote Key UP",keyPress,lcdName);
				//up_down_key(1);
				f->zoneS(_zone).userRequestTempChange(1);
				break;
			case 2:
				logToSD("Got Remote Key LEFT",keyPress,lcdName);
				//left_right_key(-1);
				break;         
			case 3:
				logToSD("Got Remote Key RIGHT",keyPress,lcdName);
				//left_right_key(1);
				break;
			case 4:
				logToSD("Got Remote Key DOWN",keyPress,lcdName);
				//up_down_key(-1);
				f->zoneS(_zone).userRequestTempChange(-1);
				break;
			default:
				U1_byte newTemp = (f->zoneS(_zone).runI().getFractionalCallSensTemp()+128)/256;
				if (newTemp != _lastTemp) {
					_lastTemp = newTemp;
					//doRefresh = keypad->isTimeToRefresh();
				} else doRefresh = false;
				if (processReset) logToSD("isTimeToRefresh? ",doRefresh);
				#if defined (NO_TIMELINE) && defined (ZPSIM)
					{static bool test = true;// for testing, prevent refresh after first time through unless key pressed
					doRefresh = test;
					test = false;
					}
				#endif
		}
		if (doRefresh) {
			//U4_byte	lastTick = micros();

			displ_data().dspl_Parent_index = _zone;
			if (i2C->getThisI2CFrequency(lcd()->i2cAddress()) == 0) {
				resetI2C(*i2C,lcd()->i2cAddress());
			} else {
				U1_byte error = streamToLCD(false);
				if (error > 0) {
					logToSD("Remote_display::processKeys() failed: ",(int)error);
					resetI2C(*i2C,lcd()->i2cAddress());
				} else {
					if (processReset) {
						logToSD("streamTo Remote LCD OK :", lcd()->i2cAddress());
						if (lcd()->i2cAddress() == 0x26) processReset = false;
					}
				}
			}
			//Serial.print("Remote_display: streamToLCD took: "); Serial.println((micros()-lastTick)/1000);
		}
		keyPress = keypad->getKey();
	} while (keyPress != -1);
	//Serial.println("Quit Remote_display::processKeys()");
	return doRefresh;
}




