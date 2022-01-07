
// *********************** Tests *********************
#include <catch.hpp>

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
using namespace client_data_structures;
using namespace Assembly;
using namespace std;


uint8_t BRIGHNESS_PWM = 5;
uint8_t CONTRAST_PWM = 6;
unsigned char PHOTO_ANALOGUE = A1;
uint8_t LOCAL_INT_PIN = 18;

//enum tableIndex { /*TB_ThermalStore, TB_MixValveContr, TB_Display, TB_Relay, TB_TempSensor, TB_TowelRail,*/ TB_Dwelling, TB_Zone, TB_DwellingZone, TB_Program, TB_Profile, TB_TimeTemp, TB_Spell, TB_NoOfTables };

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

Logger& logger() {
	static File_Logger _log("Er", 9600, clock_());
	return _log;
}

Logger& zTempLogger() {
	static File_Logger _log("ZT", 9600, clock_());
	return _log;
}

Logger& profileLogger() {
	static File_Logger _log("PR", 9600, clock_());
	return _log;
}

I2C_Talk rtc{ Wire1, 100000 };

EEPROMClassRE & eeprom() {
	static EEPROMClass_T<rtc> _eeprom_obj{ 0x50 };
	return _eeprom_obj;
}

EEPROMClassRE & EEPROM = eeprom();

constexpr TimeTemp makeTT(int hrs, int mins, int temp) {
	return { TimeOnly{ hrs, mins }, int8_t(temp) };
}

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
	,{ DateTime({ 20,1,20, },{ 5,0 }),3 } // Mon: Empty for TT's in a day
	,{ DateTime({ 22,1,20, },{ 7,30 }),1 } // Mon: Occupied for TT's in a day
	,{ DateTime({ 24,1,20, },{ 7,30 }),3 } // Mon: Occupied for TT's in a day
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
	,{ 3,3,255 }// [7] Empty Flat MTWTFSS
	,{ 1,3,255 }// [8] Occupied Flat MTWTFSS
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

	,{7, makeTT(7,30,10)}  // [16]  Empty Flat MTWTFSS
	,{8, makeTT(7,30,20)}  // [17]  Occupied Flat MTWTFSS
	,{8, makeTT(23,00,18)} // [18]  Occupied Flat MTWTFSS
};


#ifdef DATABASE
	SCENARIO("Create a Database", "[Database]") {

	RDB<TB_NoOfTables> db(RDB_START_ADDR, EEPROM_SIZE, writer, reader, 255);

	db.createTable<R_ThermalStore>(1);
	db.createTable<R_MixValveControl>(1);
	db.createTable<R_Display>(1);
	db.createTable<R_Relay>(1);
	db.createTable<R_TempSensor>(1);
	db.createTable<R_TowelRail>(1);
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
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, 255);
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
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, 255);
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
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, 255);
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

TEST_CASE("New Spell starts at curr Spell TT-time", "[Sequencer]") {
	auto zoneID = 3;
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, 255);
	HeatingSystem_Queries queries{ db };
	Sequencer sequencer{ queries };
	auto nextEvent = spells_f[4].date;
	int ttIndex[] = { 16,16, 16,17,18, 17, 18, 16, 16,16 };
	logger() << "Profile for Flat on " << nextEvent.date() << L_endl;
	for (int i = 21, t = 0; i < 29; ++i, ++t) {
		logger() << "Check " << i << " " << timeTemps_f[ttIndex[t]] << L_endl;
		auto info = sequencer.getProfileInfo(zoneID, nextEvent);
		CHECK(info.currTT.time() == timeTemps_f[ttIndex[t]].time());
		CHECK(info.nextTT.time() == timeTemps_f[ttIndex[t + 1]].time());
		nextEvent = info.nextEvent;
	}
}

