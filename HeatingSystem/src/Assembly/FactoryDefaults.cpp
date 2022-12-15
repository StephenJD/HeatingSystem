#include "FactoryDefaults.h"

#include "..\Client_DataStructures\Data_ThermalStore.h"
#include "..\Client_DataStructures\Data_MixValveControl.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Program.h"
#include "..\Client_DataStructures\Data_TimeTemp.h"
#include "..\Client_DataStructures\Data_Spell.h"
#include "..\Client_DataStructures\Data_Profile.h"
#include "..\Client_DataStructures\Data_Zone.h"
#include <LCD_Display.h>
#include <Logging.h>
#include <EEPROM_RE.h>

using namespace arduino_logger;

namespace Assembly {
	using namespace client_data_structures;

	constexpr R_ThermalStore thermalStore_f[] = {
		{R_Gas
		,T_GasF
		,T_TkTop
		,T_TkMixFl
		,T_TkLower
		,T_CWin
		,T_Pdhw
		,T_DHW
		,T_OS
		,85,21,75,25,60,180,169,138,97,25}
	};

	constexpr R_MixValveControl mixValveControl_f[] = {
		{"US", US_FLOW_TEMPSENS_ADDR, T_TkMixFl}
		, {"DS", DS_FLOW_TEMPSENS_ADDR, T_TkMixFl}
	};

	constexpr R_Display displays_f[] = {
		{"Main", 0, 16, 20, 16, 250, 70, 3}
		,{"US", US_CONSOLE_I2C_ADDR, 0, 0, 0, 0, 0, 67}
		,{"DS", DS_CONSOLE_I2C_ADDR, 0, 0, 0, 0, 0, 67}
		,{"Flat", FL_CONSOLE_I2C_ADDR, 0, 0, 0, 0, 0, 67}
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
		{ "HsTR",0x71 },
		{ "EnST",0x76 },
		{ "FlTR",0x48 },
		{ "GasF",0x4B },
		{ "OutS",0x2B },

		{ "Pdhw",0x37 },
		{ "DHot",0x28 },
		{ "TkTp",0x2D },
		{ "TkMx",0x2E },
		{ "TkLw",0x77 },
		{ "Grnd",0x35 },

		{ "TMfl",0x75 },
		{ "MFBF",0x2F }
	};
	
	constexpr R_TowelRail towelRails_f[] = {
		  { "EnSuite", T_ETrS, R_HsTR, M_UpStrs, 50, 60 }
		, { "Family", T_HTrS, R_HsTR, M_UpStrs, 50, 60 }
		, { "Flat", T_FTrS, R_FlTR, M_UpStrs, 50, 60 }
	};

	constexpr R_Dwelling dwellings_f[] = {
		{ "House" }
		,{ "HolAppt" }
	};

	constexpr R_Zone zones_f[] = {				// maxT, offsetT, autoRatio, autoTimeC, autoQuality, autoDelay;
		{ "UpStrs", "US", US_ROOM_TEMPSENS_ADDR, R_UpSt, M_UpStrs,  55,	0,			135,	220,		1,			95 }
		,{ "DnStrs","DS", DS_ROOM_TEMPSENS_ADDR, R_DnSt, M_DownStrs,55,	0,			104,	202,		1,			65 }
		,{ "Flat",  "Flt",FL_ROOM_TEMPSENS_ADDR, R_Flat, M_UpStrs,  55,	0,			104,	206,		1,			48 }
		,{ "DHW",   "DHW",T_GasF, R_Gas, 0,		  80,	0,			254,	65,			1,			5 }
	};

	constexpr R_DwellingZone dwellingZones_f[] = {
		// DwID,ZID
		{ 1,2 }
		,{ 0,0 }
		,{ 0,1 }
		,{ 1,3 } 
		,{ 0,3 }
	};

	constexpr R_Program programs_f[] = { // Name , Dwelling
		{ "At Home",0 }		// 0
		,{ "Occup'd",1 }	// 1
		,{ "At Work",0 }	// 2
		,{ "Empty",1 }		// 3
		,{ "Away",0 }		// 4
	};

	constexpr R_Spell spells_f[] = { // date, ProgramID : Ordered by date
	{ DateTime({ 31,7,19, },{ 15,20 }),0 } // at home
	,{ DateTime({ 31,7,19, },{ 15,30 }),1 }// occupied
	,{ DateTime({ 3,9,19, },{ 17,30 }),2 } // at work
	,{ DateTime({ 12,9,19, },{ 7,30 }),3 } // empty
	,{ DateTime({ 22,9,19, },{ 15,0 }),4 } // away
	,{ DateTime({ 30,9,19, },{ 10,0 }),0 } // at home
	};

	constexpr R_Profile profiles_f[] = {
		//ProgID, ZnID, Days
		{ 0,0,255 } // [0] At Home US MTWTFSS
		,{ 0,1,124 }// [1] At Home DS MTWTF--
		,{ 0,1,3 }  // [2] At Home DS -----SS
		,{ 0,3,124 }// [3] At Home DHW MTWTF--
		,{ 0,3,3 }  // [4] At Home DHW -----SS

		,{ 2,0,255 }// [5] At Work US MTWTFSS
		,{ 2,1,124 }// [6] At Work DS MTWTF--
		,{ 2,1,3 }  // [7] At Work DS -----SS
		,{ 2,3,124 }// [8] At Work DHW MTWTF--
		,{ 2,3,3 }  // [9] At Work DHW -----SS		

		,{ 4,0,255 } // [10] Away US MTWTFSS
		,{ 4,1,255 } // [11] Away DS MTWTFSS
		,{ 4,3,255 } // [12] Away DHW MTWTFSS

		,{ 1,3,255 } // [13] Occupied DHW MTWTFSS
		,{ 1,2,255}  // [14] Occupied Flat MTWTFSS
		,{ 3,3,255 } // [15] Empty DHW 
		,{ 3,2,255}  // [16] Empty Flat 
	};

	constexpr TimeTemp makeTT(int hrs, int mins, int temp) {
		return { TimeOnly{ hrs, mins }, int8_t(temp) };
	}

	R_TimeTemp timeTemps_f[] = { // profileID,TT
		{0, makeTT(7,30,15)}   // At Home US MTWTFSS
		,{0, makeTT(23,00,18)} // At Home US MTWTFSS
		,{1, makeTT(7,45,19)}  // At Home DS MTWTF--
		,{1, makeTT(23,00,16)} // At Home DS MTWTF--
		,{2, makeTT(8,00,19)}  // At Home DS -----SS
		,{2, makeTT(22,50,16)} // At Home DS -----SS
		,{3, makeTT(6,30,45)}  // At Home DHW MTWTF--
		,{3, makeTT(9,00,30)}  // At Home DHW MTWTF--
		
		,{3, makeTT(15,30,45)} // At Home DHW MTWTF--
		,{3, makeTT(22,30,30)} // At Home DHW MTWTF--
		,{4, makeTT(7,30,45)}  // At Home DHW -----SS
		,{4, makeTT(9,30,30)}  // At Home DHW -----SS
		,{4, makeTT(15,00,45)} // At Home DHW -----SS
		,{4, makeTT(22,30,30)} // At Home DHW -----SS
		,{5, makeTT(6,30,15)} // At Work US MTWTFSS
		,{5, makeTT(23,00,18)} // At Work US MTWTFSS

		,{6, makeTT(17,30,19)} // At Work DS MTWTF--
		,{6, makeTT(23,00,16)} // At Work DS MTWTF--
		,{7, makeTT(8,00,19)} // At Work DS -----SS
		,{7, makeTT(23,00,16)} // At Work DS -----SS
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
		,{14, makeTT(07,00,19)} // Occupied Flat MTWTFSS 
		,{14, makeTT(23,00,18)} // Occupied Flat MTWTFSS 
		,{15, makeTT(07,00,10)} // Empty DHW 
		,{16, makeTT(07,00,10)} // Empty Flat  
	};

	void setFactoryDefaults(RDB<TB_NoOfTables> & db, size_t password) {
		//enum tableIndex { TB_ThermalStore, TB_MixValveContr, TB_Display, TB_Relay, TB_TempSensor, TB_TowelRail, TB_Dwelling, TB_Zone, TB_DwellingZone, TB_Program, TB_Profile, TB_TimeTemp, TB_Spell, TB_NoOfTables };
		if (db.reset_OK(password, RDB_MAX_SIZE)) {
			logger() << L_time << F("setFactoryDefaults Started...\n");
			logger() << F("ThermalStore Table\n");
			db.createTable(thermalStore_f);
			logger() << F("\nMixValveContr Table\n");
			db.createTable(mixValveControl_f);
			logger() << F("\nDisplay Table\n");
			db.createTable(displays_f);
			logger() << F("\nRelays Table\n");
			db.createTable(relays_f);
			logger() << F("\nTempSensors Table\n");
			db.createTable(tempSensors_f);
			logger() << F("\nTowelRails Table\n");
			db.createTable(towelRails_f);
			logger() << F("\nDwellings Table\n");
			db.createTable(dwellings_f);
			logger() << F("\nZones Table\n");
			db.createTable(zones_f);
			logger() << F("\nDwellingZones Table\n");
			db.createTable(dwellingZones_f);
			logger() << F("\nPrograms Table\n");
			db.createTable(programs_f);
			logger() << F("\nProfiles Table\n");
			db.createTable(profiles_f);
			logger() << F("\nTimeTemps Table\n");
			db.createTable(timeTemps_f, i_09_orderedInsert);
			logger() << F("\nSpells Table\n");
			db.createTable(spells_f, i_09_orderedInsert);
			logger() << F("\nAll Tables Created\n");

			logger() << F("\nsetFactoryDefaults Completed") << L_endl;
		}
	}
	
}
