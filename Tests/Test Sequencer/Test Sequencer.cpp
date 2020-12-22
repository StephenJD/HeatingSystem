
// *********************** Tests *********************
#include <catch.hpp>

#include "HeatingSystemEnums.h"
#include "HeatingSystem_Queries.h"
#include "Sequencer.h"
#include "DateTimeWrapper.h"
#include "Data_Dwelling.h"
#include "Data_CurrentTime.h"
#include "Data_MixValveControl.h"
#include "Data_Program.h"
#include "Data_Profile.h"
#include "Data_Relay.h"
#include "Data_Spell.h"
#include "Data_TempSensor.h"
#include "Data_ThermalStore.h"
#include "Data_TimeTemp.h"
#include "Data_TowelRail.h"
#include "Data_Zone.h"
#include <EEPROM.h>
#include <Clock_.h>
#include <Conversions.h>
#include <RDB.h>
#include <Logging.h>
#include "TempSensor.h"
#include <iostream>
#include <iomanip>

#define DATABASE

using namespace RelationalDatabase;
using namespace HardwareInterfaces;
using namespace GP_LIB;
using namespace Date_Time;
using namespace std;


uint8_t BRIGHNESS_PWM = 5;
uint8_t CONTRAST_PWM = 6;
unsigned char PHOTO_ANALOGUE = A1;
uint8_t LOCAL_INT_PIN = 18;

using namespace client_data_structures;
using namespace Assembly;

int writer(int address, const void* data, int noOfBytes) {
	if (address < RDB_START_ADDR || address + noOfBytes > RDB_START_ADDR + RDB_MAX_SIZE) {
		logger() << F("Illegal RDB write address: ") << int(address) << L_endl;
		return address;
	}
	const unsigned char* byteData = static_cast<const unsigned char*>(data);
	for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
		eeprom().update(address, *byteData);
	}
	return address;
}

int reader(int address, void* result, int noOfBytes) {
	if (address < RDB_START_ADDR || address + noOfBytes > RDB_START_ADDR + RDB_MAX_SIZE) {
		logger() << F("Illegal RDB read address: ") << int(address) << L_endl;
		return address;
	}
	uint8_t* byteData = static_cast<uint8_t*>(result);
	for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {

		* byteData = eeprom().read(address);
	}
	return address;
}

std::string	test_stream(UI_DisplayBuffer & buffer) {
	std::string result = buffer.toCStr();
	if (buffer.cursorPos() >= (int)result.length()) {
		cout << "\nERROR in test_stream : cursorPos beyond last field\n\n";
		return result + " ** ERROR : cursorPos beyond last field **";
	}
	switch (buffer.cursorMode()) {
	case LCD_Display::e_selected:
		result.insert(buffer.cursorPos(), "_");
		break;
	case LCD_Display::e_inEdit:
		result.insert(buffer.cursorPos(), "#");
		break;
	default:
		break;
	}
	return result;
}

namespace HeatingSystemSupport {
	int writer(int address, const void * data, int noOfBytes);
	int reader(int address, void * result, int noOfBytes);
}

Logger & logger() {
	static Serial_Logger _log(9600, clock_());
	return _log;
}

Logger & zTempLogger() {
	static Serial_Logger _log(9600, clock_());
	return _log;
}

Logger & mTempLogger() {
	static Serial_Logger _log(9600, clock_());
	return _log;
}

I2C_Talk rtc{ Wire1, 100000 };

EEPROMClass & eeprom() {
	static EEPROMClass_T<rtc> _eeprom_obj{ 0x50 };
	return _eeprom_obj;
}

EEPROMClass & EEPROM = eeprom();

constexpr TimeTemp makeTT(int hrs, int mins, int temp) {
	return { TimeOnly{ hrs, mins }, int8_t(temp) };
}

constexpr R_ThermalStore thermalStore_f[] = {
	{R_Gas
	,T_GasF
	,T_TkTop
	,T_TkUs
	,T_TkDs
	,T_CWin
	,T_Pdhw
	,T_DHW
	,T_OS
	,85,21,75,25,60,180,169,138,97,25}
};

constexpr R_MixValveControl mixValveControl_f[] = {
	{"DnS", T_DUfh, T_TkDs}
	,{"UpS", T_UUfh, T_TkUs}
};

constexpr R_Display displays_f[] = {
	{"Main", 0, 16, 20, 16, 250, 70, 30}
	,{"US", US_REMOTE_ADDRESS, 0, 0, 0, 0, 0, 0}
	,{"DS", DS_REMOTE_ADDRESS, 0, 0, 0, 0, 0, 0}
	,{"Flat", FL_REMOTE_ADDRESS, 0, 0, 0, 0, 0, 0}
};

constexpr R_Relay relays_f[] = {
	{ "Flat",6 << 2 }
	,{ "FlTR",1 << 2 }
	,{ "HsTR",0 << 2 }
	,{ "UpSt",5 << 2 }
	,{ "MFSt",2 << 2 }
	,{ "Gas",3 << 2 }
	,{ "DnSt",4 << 2 }
};

constexpr R_TempSensor tempSensors_f[] = {
	{ "UpSt",0x36 },
	{ "DnSt",0x74 },
	{ "Flat",0x70 },
	{ "HsTR",0x71 },
	{ "EnST",0x76 },
	{ "FlTR",0x48 },
	{ "GasF",0x4B },
	{ "OutS",0x2B },

	{ "Pdhw",0x37 },
	{ "DHot",0x28 },
	{ "US-F",0x2C },
	{ "DS-F",0x4F },
	{ "TkTp",0x2D },
	{ "TkUs",0x2E },
	{ "TkDs",0x77 },
	{ "Grnd",0x35 },

	{ "TMfl",0x75 },
	{ "MFBF",0x2F }
};

constexpr R_TowelRail towelRails_f[] = {
		{ "EnSuite", T_ETrS, R_HsTR, M_UpStrs, 50, 60 }
	, { "Family", T_HTrS, R_HsTR, M_UpStrs, 51, 61 }
	, { "Flat", T_FTrS, R_FlTR, M_UpStrs, 52, 62 }
};

constexpr R_Dwelling dwellings_f[] = {
	{ "House" }
	,{ "HolAppt" }
};

constexpr R_Zone zones_f[] = {
	{ "UpStrs", "US", T_UR,R_UpSt, M_UpStrs,  55,0,102,216,1,194 }
	,{ "DnStrs","DS", T_DR,R_DnSt, M_DownStrs,45,0,125,192,1,173 }
	,{ "DHW",   "DHW",T_GasF,R_Gas,M_UpStrs,  80,0,0,35,1,35 }
	,{ "Flat",  "Flt",T_FR,R_Flat, M_UpStrs,  55,0,107,217,1,194 }
};

constexpr R_DwellingZone dwellingZones_f[] = {
	{ 1,3 }
	,{ 0,0 }
	,{ 0,1 }
	,{ 1,2 }
	,{ 0,2 }
};

constexpr R_Program programs_f[] = {
	{ "At Home",0 }		// 0
	,{ "Occup'd",1 }	// 1
	,{ "At Work",0 }	// 2
	,{ "Empty",1 }		// 3
	,{ "Away",0 }		// 4
};

constexpr R_Spell spells_f[] = { // date, ProgramID : Ordered by date
	{ DateTime({ 6,1,20, },{ 7,0 }),0 } // Monday: At Home for TT's in a day
	,{ DateTime({ 8,1,20, },{ 7,0 }),0 } // Wed: At Home for TT's in a day
	,{ DateTime({ 13,1,20, },{ 5,0 }),1 } // Mon: Occupied for TT's in a day
	,{ DateTime({ 13,1,20, },{ 5,0 }),2 } // Mon: At Work for TT's in a day
};

constexpr R_Profile profiles_f[] = {
	//ProgID, ZnID, Days
	{ 0,1,96 }  // [0] At Home DS MT----- // Multiple TT's in a day
	,{ 0,1,16 } // [1] At Home DS --W----
	,{ 0,1,8 }  // [2] At Home DS ---T---
	,{ 0,1,4 }  // [3] At Home DS ----F--

	,{ 0,1,3 }  // [4] At Home  DS  -----SS
	,{ 1,2,255 }// [5] Occupied DHW MTWTFSS
	,{ 2,2,255 }// [6] At Work  DHW MTWTFSS
	,{ 2,1,255 }// [7] At Work  DS  MTWTFSS

	,{ 4,0,124 }// [8] At Work DHW MTWTF--
	,{ 4,0,3 }  // [9] At Work DHW -----SS		
	,{ 4,0,255 } // [10] Away US MTWTFSS
	,{ 4,0,255 } // [11] Away DS MTWTFSS

	,{ 4,0,255 } // [12] Away DHW MTWTFSS
	,{ 4,0,255 } // [13] Occupied DHW MTWTFSS
	,{ 4,0,255}  // [14] Occupied Flat MTWTFSS
	,{ 4,0,255 } // [15] Empty DHW 
	,{ 4,0,255}  // [16] Empty Flat 
};


R_TimeTemp timeTemps_f[] = { // profileID,TT
	{0, makeTT(7,0,15)}    // At Home DS MT-----
	,{0, makeTT(8,10,19)}  // At Home DS MT-----
	,{0, makeTT(9,20,15)}  // At Home DS MT-----
	,{0, makeTT(18,00,19)} // At Home DS MT-----
	,{1, makeTT(15,00,20)} // At Home DS --W----
	,{2, makeTT(13,30,12)} // At Home DS ---T---
	,{3, makeTT(11,50,18)} // At Home DS ----F--
	,{4, makeTT(7,00,15)}  // At Home DS -----SS

	,{5, makeTT(7,30,50)}  // [8]  Occupied DHW MTWTFSS
	,{5, makeTT(9,30,37)}  // [9]  Occupied DHW MTWTFSS
	,{5, makeTT(15,30,45)} // [10] Occupied DHW MTWTFSS
	,{5, makeTT(22,00,30)} // [11] Occupied DHW MTWTFSS
	,{6, makeTT(6,30,45)}  // [12] At Work  DHW MTWTFSS
	,{6, makeTT(8,00,40)}  // [13] At Work  DHW MTWTFSS
	,{6, makeTT(14,00,35)} // [14] At Work  DHW MTWTFSS
	,{6, makeTT(23,00,30)} // [15] At Work  DHW MTWTFSS

	,{7, makeTT(7,30,16)}  // At Work DS MTWTFSS
	,{7, makeTT(18,00,19)} // At Work DS MTWTFSS
	,{8, makeTT(8,00,19)}  // At Work DS -----SS
	,{8, makeTT(23,00,16)} // At Work DS -----SS
	,{8, makeTT(06,30,45)} // At Work DHW MTWTF--
	,{8, makeTT(07,00,30)} // At Work DHW MTWTF--
	,{8, makeTT(18,00,45)} // At Work DHW MTWTF--
	,{8, makeTT(22,30,30)} // At Work DHW MTWTF--

	,{9, makeTT(7,40,45)}  // At Work DHW -----SS
	,{9, makeTT(10,00,30)}  // At Work DHW -----SS 
	,{9, makeTT(15,30,45)} // At Work DHW -----SS 
	,{9, makeTT(23,00,30)} // At Work DHW -----SS 
	,{10, makeTT(07,00,10)} // Away US MTWTFSS 
	,{11, makeTT(07,00,10)} // Away DS MTWTFSS 
	,{12, makeTT(07,00,10)} // Away DHW MTWTFSS 
	,{13, makeTT(07,00,45)} // Occupied DHW MTWTFSS 

	,{13, makeTT(10,00,30)} // Occupied DHW MTWTFSS 
	,{13, makeTT(16,00,45)} // Occupied DHW MTWTFSS 
	,{13, makeTT(23,00,30)} // Occupied DHW MTWTFSS 
	,{14, makeTT(07,00,20)} // Occupied Flat MTWTFSS 
	,{14, makeTT(23,00,18)} // Occupied Flat MTWTFSS 
	,{15, makeTT(07,00,10)} // Empty DHW 
	,{16, makeTT(07,00,10)} // Empty Flat  
};


#ifdef DATABASE
	SCENARIO("Create a Database", "[Database]") {

	RDB<TB_NoOfTables> db(RDB_START_ADDR, EEPROM_SIZE, writer, reader, VERSION);

	db.createTable<>(thermalStore_f);
	db.createTable(mixValveControl_f);
	db.createTable(displays_f);
	db.createTable(relays_f);
	db.createTable(tempSensors_f);
	db.createTable(towelRails_f);
	db.createTable(dwellings_f);
	db.createTable(zones_f);
	db.createTable(dwellingZones_f);
	db.createTable(programs_f);
	db.createTable(profiles_f);
	db.createTable(timeTemps_f, i_09_orderedInsert);
	db.createTable(spells_f, i_09_orderedInsert);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Profile repeated over 2 Days", "[Sequencer]") {
	auto zoneID = 1;
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, VERSION);
	HeatingSystem_Queries queries{db};
	Sequencer sequencer{queries};
	auto nextEvent = spells_f[0].date;
	auto nextDay = nextEvent.date();
	auto correctNextDate = nextEvent;
	int ttIndex[] = { 0,1,2,3, 0,1,2,3, 3 };
	logger() << "Profile for DS\n";
	for (int i = 0, t = 0; i < 8; ++i,++t) {
		if (i == 3 || i == 7) nextDay.addDays(1);
		logger() << "Check " << i << " " << timeTemps_f[ttIndex[t]] << L_endl;
		auto info = sequencer.getProfileInfo(zoneID, nextEvent);
		correctNextDate.date() = nextDay;
		CHECK(info.currTT.time() == timeTemps_f[ttIndex[t]].time());
		CHECK(info.nextTT.time() == timeTemps_f[ttIndex[t+1]].time());
		CHECK(info.nextEvent.date() == correctNextDate);
		nextEvent = info.nextEvent;
	}
}

TEST_CASE("Profiles get earlier over 2 Days", "[Sequencer]") {
	auto zoneID = 1;
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, VERSION);
	HeatingSystem_Queries queries{ db };
	Sequencer sequencer{ queries };
	auto nextEvent = spells_f[1].date;
	auto nextDay = nextEvent.date();
	auto correctNextDate = nextEvent;
	int ttIndex[] = {3,4,5,6, 7,7,16};
	logger() << "Profile for DS\n";
	for (int i = 8, t = 0; i < 13; ++i,++t) {
		logger() << "Check " << i << " " << timeTemps_f[ttIndex[t]] << L_endl;
		auto info = sequencer.getProfileInfo(zoneID, nextEvent);
		correctNextDate.date() = nextDay;
		CHECK(info.currTT.time() == timeTemps_f[ttIndex[t]].time());
		CHECK(info.nextTT.time() == timeTemps_f[ttIndex[t + 1]].time());
		CHECK(info.nextEvent.date() == correctNextDate);
		nextEvent = info.nextEvent;
		nextDay.addDays(1);
	}
}

TEST_CASE("Two-dwellings Higher takes priority", "[Sequencer]") {
	auto zoneID = 2;
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, VERSION);
	HeatingSystem_Queries queries{ db };
	Sequencer sequencer{ queries };
	auto nextEvent = spells_f[2].date;
	auto nextDay = nextEvent.date();
	auto correctNextDate = nextEvent;
	logger() << "Profile for DHW\n";
	int temps[] = {11,12,8,13,9,10,14,11,12};
	int times[] = { 11,12,8,13,9,10,14,11,12};
	for (int i = 13, t = 0; i < 21; ++i,++t) {
		logger() << "Check " << i << L_endl;
		auto info = sequencer.getProfileInfo(zoneID, nextEvent);
		if (i == 20) nextDay.addDays(1);
		correctNextDate.date() = nextDay;
		CHECK((int)info.currTT.temp() == (int)timeTemps_f[temps[t]].temp());
		CHECK((int)info.nextTT.temp() == (int)timeTemps_f[temps[t + 1]].temp());
		CHECK(info.currTT.time() == timeTemps_f[times[t]].time());
		CHECK(info.nextTT.time() == timeTemps_f[times[t + 1]].time());
		CHECK(info.nextEvent.date() == correctNextDate);
		nextEvent = info.nextEvent;
	}
}
