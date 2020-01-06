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
		,{"DS", 0x26, 0, 0, 0, 0, 0, 0}
		,{"US", 0x24, 0, 0, 0, 0, 0, 0}
		,{"Flat", 0x25, 0, 0, 0, 0, 0, 0}
	};

	constexpr R_Relay relays_f[] = {
		{ "Flat",6 << 1 }
		,{ "FlTR",1 << 1 }
		,{ "HsTR",0 << 1 }
		,{ "UpSt",5 << 1 }
		,{ "MFSt",2 << 1 }
		,{ "Gas",3 << 1 }
		,{ "DnSt",4 << 1 }
	};

	constexpr R_TempSensor tempSensors_f[] = {
		{ "Flat",0x70 },
		{ "FlTR",0x72 },
		{ "HsTR",0x71 },
		{ "EnST",0x76 },
		{ "UpSt",0x36 },
		{ "DnSt",0x74 },
		{ "OutS",0x2B }, // { "OutS",0x2B }, // 29
		{ "Grnd",0x35 },

		{ "Pdhw",0x37 },
		{ "DHot",0x28 },
		{ "US-F",0x2C },
		{ "DS-F",0x4F },
		{ "TkMf",0x75 },
		{ "TkDs",0x77 },
		{ "TkUs",0x2E },
		{ "TkTp",0x2D },

		{ "GasF",0x4B },
		{ "MFBF",0x2F }
	};

	constexpr R_Dwelling dwellings_f[] = {
		{ "House" }
		,{ "HolAppt" }
	};

	constexpr R_Program programs_f[] = {
		{ "At Home",0 }
		,{ "Occup'd",1 }
		,{ "At Work",0 }
		,{ "Empty",1 }
		,{ "Away",0 }
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
		,{ DateTime({ 12,9,19, },{ 7,30 }),1 }
		,{ DateTime({ 3,9,19, },{ 17,30 }),2 }
		,{ DateTime({ 5,9,19, },{ 10,0 }),3 }
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
		//enum tableIndex {TB_Display, TB_Relay, TB_TempSensor, TB_Dwelling, TB_Program, TB_DwellingZone, TB_TimeTemp, TB_Spell, TB_Profile, TB_Zone, TB_NoOfTables };
		db.reset(EEPROM_SIZE, password);
		logger() << L_time << "setFactoryDefaults Started...";
		logger() << "\nThermalStore Table ";
		db.createTable(thermalStore_f);
		logger() << "\nMixValveContr Table ";
		db.createTable(mixValveControl_f);
		logger() << "\nDisplay Table ";
		db.createTable(displays_f);
		logger() << "\nRelays Table ";
		db.createTable(relays_f);
		logger() << "\nTempSensors Table ";
		db.createTable(tempSensors_f);
		logger() << "\nDwellings Table ";
		db.createTable(dwellings_f);
		logger() << "\nZones Table ";
		db.createTable(zones_f);
		logger() << "\nDwellingZones Table ";
		db.createTable(dwellingZones_f);
		logger() << "\nPrograms Table ";
		db.createTable(programs_f);
		logger() << "\nProfiles Table ";
		db.createTable(profiles_f);
		logger() << "\nTimeTemps Table ";
		db.createTable(timeTemps_f, i_09_orderedInsert);
		logger() << "\nSpells Table ";
		db.createTable(spells_f, i_09_orderedInsert);
		logger() << "\nAll Tables Created";

		logger() << "\n\nsetFactoryDefaults Completed" << L_endl;
	}
	
}
