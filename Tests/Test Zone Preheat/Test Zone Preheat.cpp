
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
#include "Zone.h"
#include <EEPROM.h>
#include <Clock_.h>
#include <Conversions.h>
#include <RDB.h>
#include <Logging.h>
#include "TempSensor.h"
#include "I2C_Recover.h"
#include "ThermalStore.h"
#include "BackBoiler.h"
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

void ui_yield() {}

namespace HardwareInterfaces {
	I2C_Talk i2C;

	I2C_Recovery::I2C_Recover recover{ i2C };

	Bitwise_RelayController& relayController() {
		static RelaysPort _relaysPort(0, recover, 0);
		return _relaysPort;
	}
	UI_TempSensor tempSensorArr[] = { {/*OS*/recover,1,0}, {/*DS Room*/recover,1,15} };
	MixValveController mixValveControllerArr[] = { {recover,1}, {recover,1} };
	UI_Bitwise_Relay relayArr[] = { 0 };
	BackBoiler backBoiler{ tempSensorArr[0], tempSensorArr[0], relayArr[0] };
	ThermalStore thermalStore(tempSensorArr, mixValveControllerArr, backBoiler);
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
	{ 0,1,255 }  // [0] At Home DS MTWTFSS // Multiple TT's in a day
};


R_TimeTemp timeTemps_f[] = { // profileID,TT
	{0, makeTT(7,0,15)}    // At Home DS MTWTFSS
	,{0, makeTT(8,10,19)}  // At Home DS MTWTFSS
	,{0, makeTT(9,20,15)}  // At Home DS MTWTFSS
	,{0, makeTT(12,20,14)}  // At Home DS MTWTFSS
	,{0, makeTT(18,00,21)} // At Home DS MTWTFSS
	,{0, makeTT(22,00,15)} // At Home DS MTWTFSS
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

TEST_CASE("Preheat for next warmer and cooler", "[Preheat]") {
	clock_().setTime( spells_f[0].date.date(), spells_f[0].date.time(),0 );
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, 255);
	HeatingSystem_Queries queries{db};
	Sequencer sequencer{queries};
	Zone zone{};
	Zone::setSequencer(sequencer);
	zone.initialise(queries.q_Zones[1], tempSensorArr[1], relayArr[0], thermalStore, mixValveControllerArr[0]);
	zone.zoneRecord().rec().autoTimeC = GetExpCurveConsts::compressTC(240); // 60m per degree. TC = minutes - per - 4 - degrees
	zone.zoneRecord().rec().autoRatio = 0; // linear
	zone.zoneRecord().rec().autoQuality = 1; 
	zone.zoneRecord().update();
	
	uint8_t  roomTemps[] =	{18,	18,		19,	  	18,		15,		15,		15,		15};
	uint8_t preheatTemps[] = {15,	19,		15,	  	19,		19,		19,		14,		21};
	TimeOnly     times[] = { {7,0},	{7,10},	{7,20},	{8,0},	{8,10},	{ 9,0 },{12,10},{12,20} };

	for (int t = 0; t < sizeof(roomTemps); ++t) {
		clock_().setTime(times[t]);
		tempSensorArr[1].setTestTemp(roomTemps[t]);
		cout << "\nCheck " << t << " " << clock_().now() << ": Curr Temp: " << (int)tempSensorArr[1].get_temp() << endl;
		zone.preHeatForNextTT();
		cout << "Preheat TempReq: " << (int)zone.preheatTempRequest() << endl;
		CHECK((int)zone.preheatTempRequest() == preheatTemps[t]);
		zone.zoneRecord().rec().autoTimeC = GetExpCurveConsts::compressTC(240); // 60m per degree. TC = minutes - per - 4 - degrees
		zone.zoneRecord().rec().autoRatio = 0; // linear
		zone.zoneRecord().rec().autoQuality = 1;
		zone.zoneRecord().update();
	}
}

TEST_CASE("Preheat for later warmer", "[Preheat]") {
	clock_().setTime( spells_f[0].date.date(), spells_f[0].date.time(),0 );
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, 255);
	HeatingSystem_Queries queries{db};
	Sequencer sequencer{queries};
	Zone zone{};
	Zone::setSequencer(sequencer);
	zone.initialise(queries.q_Zones[1], tempSensorArr[1], relayArr[0], thermalStore, mixValveControllerArr[0]);
	zone.zoneRecord().rec().autoTimeC = GetExpCurveConsts::compressTC(240); // 60m per degree. TC = minutes - per - 4 - degrees
	zone.zoneRecord().rec().autoRatio = 0; // linear
	zone.zoneRecord().rec().autoQuality = 1;
	zone.zoneRecord().update();
	
	uint8_t  roomTemps[] =	{18,	15,		15,		15,		15};
	uint8_t preheatTemps[] = {15,	19,		15,		21,		21};
	TimeOnly     times[] = { {7,0},	{7,10},{11,50}, { 12,0 }, {18,0} };

	for (int t = 0; t < sizeof(roomTemps); ++t) {
		clock_().setTime(times[t]);
		tempSensorArr[1].setTestTemp(roomTemps[t]);
		cout << "\nCheck " << t+8 << "\n" << clock_().now() << ": Curr Temp: " << (int)tempSensorArr[1].get_temp() << endl;
		zone.preHeatForNextTT();
		cout << "Preheat TempReq: " << (int)zone.preheatTempRequest() << endl << endl;
		CHECK((int)zone.preheatTempRequest() == preheatTemps[t]);
		zone.zoneRecord().rec().autoTimeC = GetExpCurveConsts::compressTC(240); // 60m per degree. TC = minutes - per - 4 - degrees
		zone.zoneRecord().rec().autoRatio = 0; // linear
		zone.zoneRecord().rec().autoQuality = 1;
		zone.zoneRecord().update();
	}
}

TEST_CASE("Preheat when advanced to next profile", "[Preheat]") {
	clock_().setTime( spells_f[0].date.date(), spells_f[0].date.time(),0 );
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, 255);
	HeatingSystem_Queries queries{db};
	Sequencer sequencer{queries};
	Zone zone{};
	Zone::setSequencer(sequencer);
	zone.initialise(queries.q_Zones[1], tempSensorArr[1], relayArr[0], thermalStore, mixValveControllerArr[0]);
	zone.zoneRecord().rec().autoTimeC = GetExpCurveConsts::compressTC(240); // 60m per degree. TC = minutes - per - 4 - degrees
	zone.zoneRecord().rec().autoRatio = 0; // linear
	zone.zoneRecord().rec().autoQuality = 1;
	zone.zoneRecord().update();
	
	uint8_t  roomTemps[] = { 18,	18,		19,	  	18,		15,		15,		15,		15 };
	uint8_t preheatTemps[] = { 19,	19,		19,	  	19,		19,		19,		14,		21 };
	TimeOnly     times[] = { {7,0},	{7,10},	{7,20},	{8,0},	{8,10},	{9,0},{12,10},{12,20} };
	zone.preHeatForNextTT();
	zone.setProfileTempRequest(zone.nextTempRequest()); // advance to next profile : 8:10 @ 19

	for (int t = 0; t < sizeof(roomTemps); ++t) {
		clock_().setTime(times[t]);
		tempSensorArr[1].setTestTemp(roomTemps[t]);
		cout << "\nCheck " << t+13 << "\n" << clock_().now() << ": Curr Temp: " << (int)tempSensorArr[1].get_temp() << endl;
		zone.preHeatForNextTT();
		cout << "Preheat TempReq: " << (int)zone.preheatTempRequest() << endl << endl;
		CHECK((int)zone.preheatTempRequest() == preheatTemps[t]);
		zone.zoneRecord().rec().autoTimeC = GetExpCurveConsts::compressTC(240); // 60m per degree. TC = minutes - per - 4 - degrees
		zone.zoneRecord().rec().autoRatio = 0; // linear
		zone.zoneRecord().rec().autoQuality = 1;
		zone.zoneRecord().update();
	}
}

TEST_CASE("Learn warm-up rate", "[Preheat]") {
	clock_().setTime( spells_f[0].date.date(), spells_f[0].date.time(),0 );
	RDB<TB_NoOfTables> db(RDB_START_ADDR, writer, reader, 255);
	HeatingSystem_Queries queries{db};
	Sequencer sequencer{queries};
	Zone zone{};
	Zone::setSequencer(sequencer);
	zone.initialise(queries.q_Zones[1], tempSensorArr[1], relayArr[0], thermalStore, mixValveControllerArr[0]);
	zone.zoneRecord().rec().autoTimeC = GetExpCurveConsts::compressTC(240); // 60m per degree. TC = minutes - per - 4 - degrees
	zone.zoneRecord().rec().autoRatio = 0; // linear
	zone.zoneRecord().rec().autoQuality = 1;
	zone.zoneRecord().update();
	cout << "\nLearn warm - up rate" << endl;
	
	while (clock_().now().time() < TimeOnly{ 18,20 }) {
		cout << clock_().now() << ": Curr Temp: " << tempSensorArr[1].get_fractional_temp()/256. << endl;
		zone.preHeatForNextTT();
		cout << "Preheat TempReq: " << (int)zone.preheatTempRequest() << endl << endl;
		if (zone.preheatTempRequest() > tempSensorArr[1].get_temp()) {
			auto newRoomTemp = double(tempSensorArr[1].get_fractional_temp()) / 256 + 0.333;
			if (newRoomTemp > 21) newRoomTemp = 21;
			tempSensorArr[1].setTestTemp(newRoomTemp);
		}
		clock_().setDateTime(DateTime(clock_().now()).addOffset({ m10,1 }));
	}
	CHECK((int)zone.zoneRecord().rec().autoTimeC == (int)GetExpCurveConsts::compressTC(120)); // 30m per degree. TC = minutes - per - 4 - degrees
}

