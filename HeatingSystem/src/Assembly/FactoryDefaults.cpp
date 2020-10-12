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
#include "..\HardwareInterfaces\LCD_Display.h"
#include <Logging.h>
#include <EEPROM.h>

namespace Assembly {
	using namespace client_data_structures;

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
	
	//constexpr R_TempSensor tempSensors_f[] = {
	//	{ "Flat",0x29 },
	//	{ "FlTR",0x29 },
	//	{ "HsTR",0x29 },
	//	{ "EnST",0x29 },
	//	{ "UpSt",0x29 },
	//	{ "DnSt",0x29 },
	//	{ "OutS",0x29 },
	//	{ "Grnd",0x29 },

	//	{ "Pdhw",0x29 },
	//	{ "DHot",0x29 },
	//	{ "US-F",0x29 },
	//	{ "DS-F",0x29 },
	//	{ "TkMf",0x29 },
	//	{ "TkDs",0x29 },
	//	{ "TkUs",0x29 },
	//	{ "TkTp",0x29 },

	//	{ "GasF",0x29 },
	//	{ "MFBF",0x29 }
	//};

	constexpr R_TowelRail towelRails_f[] = {
		  { "EnSuite", T_ETrS, R_HsTR, M_UpStrs, 50, 60 }
		, { "Family", T_HTrS, R_HsTR, M_UpStrs, 51, 61 }
		, { "Flat", T_FTrS, R_FlTR, M_UpStrs, 52, 62 }
	};

	constexpr R_Dwelling dwellings_f[] = {
		{ "House" }
		,{ "HolAppt" }
	};

	constexpr R_Program programs_f[] = {
		{ "At Home",0 }		// 0
		,{ "Occup'd",1 }	// 1
		,{ "At Work",0 }	// 2
		,{ "Empty",1 }		// 3
		,{ "Away",0 }		// 4
	};

	constexpr R_DwellingZone dwellingZones_f[] = {
		{ 1,3 }
		,{ 0,0 }
		,{ 0,1 }
		,{ 1,2 } 
		,{ 0,2 }
	};

	constexpr TimeTemp makeTT(int hrs, int mins, int temp) {
		return { TimeOnly{ hrs, mins }, int8_t(temp) };
	}

	R_TimeTemp timeTemps_f[] = { // profileID,TT
		{0, makeTT(7,30,15)}   // At Home US MTWTFSS
		,{0, makeTT(21,00,18)} // At Home US MTWTFSS
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
		,{5, makeTT(21,00,18)} // At Work US MTWTFSS

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
		,{14, makeTT(07,00,20)} // Occupied Flat MTWTFSS 
		,{14, makeTT(23,00,18)} // Occupied Flat MTWTFSS 
		,{15, makeTT(07,00,10)} // Empty DHW 
		,{16, makeTT(07,00,10)} // Empty Flat  
	};

	constexpr R_Spell spells_f[] = { // date, ProgramID : Ordered by date
		{ DateTime({ 31,7,19, },{ 15,20 }),0 }
		,{ DateTime({ 3,9,19, },{ 17,30 }),2 }
		,{ DateTime({ 5,9,19, },{ 10,0 }),1 }
		,{ DateTime({ 12,9,19, },{ 7,30 }),3 }
		,{ DateTime({ 22,9,19, },{ 15,0 }),4 }
		,{ DateTime({ 30,9,19, },{ 10,0 }),0 } 
	};

	constexpr R_Profile profiles_f[] = {
		//ProgID, ZnID, Days
		{ 0,0,255 } // [0] At Home US MTWTFSS
		,{ 0,1,124 }// [1] At Home DS MTWTF--
		,{ 0,1,3 }  // [2] At Home DS -----SS
		,{ 0,2,124 }// [3] At Home DHW MTWTF--
		,{ 0,2,3 }  // [4] At Home DHW -----SS
		
		,{ 2,0,255 }// [5] At Work US MTWTFSS
		,{ 2,1,124 }// [6] At Work DS MTWTF--
		,{ 2,1,3 }  // [7] At Work DS -----SS
		,{ 2,2,124 }// [8] At Work DHW MTWTF--
		,{ 2,2,3 }  // [9] At Work DHW -----SS		
		
		,{ 4,0,255 } // [10] Away US MTWTFSS
		,{ 4,1,255 } // [11] Away DS MTWTFSS
		,{ 4,2,255 } // [12] Away DHW MTWTFSS
		
		,{ 1,2,255 } // [13] Occupied DHW MTWTFSS
		,{ 1,3,255}  // [14] Occupied Flat MTWTFSS
		,{ 3,2,255 } // [15] Empty DHW 
		,{ 3,3,255}  // [16] Empty Flat 
	};	
		
	constexpr R_Zone zones_f[] = { 
		{ "UpStrs","US",T_UR,R_UpSt,M_UpStrs,55,0,25,12,1,60 }
		,{ "DnStrs","DS",T_DR,R_DnSt,M_DownStrs,45,0,25,12,1,60 }
		,{ "DHW","DHW",T_GasF,R_Gas,M_UpStrs,80,0,60,12,1,3 }
		,{ "Flat","Flt",T_FR,R_Flat,M_UpStrs,55,0,25,12,1,60 }
	};

	void setFactoryDefaults(RDB<TB_NoOfTables> & db, size_t password) {
		
		//logger() << F("\n\nsetFactoryDefaults Try DB Readable") << L_endl;
		//HeatingSystemSupport::initialise_virtualROM();
		//auto noOpen = db.getTables(db.begin(), TB_NoOfTables);

		//if (noOpen == TB_NoOfTables) return;
		
		//enum tableIndex { TB_ThermalStore, TB_MixValveContr, TB_Display, TB_Relay, TB_TempSensor, TB_TowelRail, TB_Dwelling, TB_Zone, TB_DwellingZone, TB_Program, TB_Profile, TB_TimeTemp, TB_Spell, TB_NoOfTables };
		db.reset(password, RDB_MAX_SIZE);
		logger() << L_time << F("setFactoryDefaults Started...\n");
		logger() << F("ThermalStore Table\n");
		db.createTable(thermalStore_f);
		logger() << F("\nMixValveContr Table ");
		db.createTable(mixValveControl_f);
		logger() << F("\nDisplay Table ");
		db.createTable(displays_f);
		logger() << F("\nRelays Table ");
		db.createTable(relays_f);
		logger() << F("\nTempSensors Table ");
		db.createTable(tempSensors_f);
		logger() << F("\nTowelRails Table ");
		db.createTable(towelRails_f);
		logger() << F("\nDwellings Table ");
		db.createTable(dwellings_f);
		logger() << F("\nZones Table ");
		db.createTable(zones_f);
		logger() << F("\nDwellingZones Table ");
		db.createTable(dwellingZones_f);
		logger() << F("\nPrograms Table ");
		db.createTable(programs_f);
		logger() << F("\nProfiles Table ");
		db.createTable(profiles_f);
		logger() << F("\nTimeTemps Table ");
		db.createTable(timeTemps_f, i_09_orderedInsert);
		logger() << F("\nSpells Table ");
		db.createTable(spells_f, i_09_orderedInsert);
		logger() << F("\nAll Tables Created");

		logger() << F("\n\nsetFactoryDefaults Completed") << L_endl;

		//logger() << F("\n\nCheck DB Readable") << L_endl;
		//HeatingSystemSupport::initialise_virtualROM();
		//db.getTables(db.begin(), TB_NoOfTables);

	}
	
}
