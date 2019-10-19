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
	, {"UpS", T_UUfh, T_TkUs}
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
		{0, makeTT(7,40,15)}   // 11801
		,{0, makeTT(10,30,18)} // 16156
		,{1, makeTT(7,30,15)}
		,{1, makeTT(22,40,18)}
		,{2, makeTT(8,30,15)}
		,{2, makeTT(22,50,18)}
		,{3, makeTT(6,30,45)}
		,{3, makeTT(10,00,30)}
		,{3, makeTT(16,00,45)}
		,{3, makeTT(23,00,30)}
		,{4, makeTT(07,00,18)}
		,{5, makeTT(07,00,18)}
		,{6, makeTT(07,00,18)}
		,{7, makeTT(07,00,45)}

		,{8, makeTT(07,00,10)}
		,{9, makeTT(07,00,10)}
		,{10, makeTT(07,00,10)}
		,{11, makeTT(07,00,10)}
		,{12, makeTT(07,00,45)}
		,{13, makeTT(07,00,18)}
		,{14, makeTT(07,00,18)}
		,{15, makeTT(07,00,45)}

		,{16, makeTT(07,00,45)}
		,{17, makeTT(07,00,45)}
		,{18, makeTT(07,00,18)}
		,{19, makeTT(07,00,18)}
		,{20, makeTT(07,00,10)}
	};

	constexpr R_Spell spells_f[] = {
		{ DateTime({ 31,7,19, },{ 15,20 }),0 }
		,{ DateTime({ 12,9,19, },{ 7,30 }),1 }
		,{ DateTime({ 3,9,19, },{ 17,30 }),2 }
		,{ DateTime({ 5,9,19, },{ 10,0 }),3 }
		,{ DateTime({ 22,9,19, },{ 15,0 }),2 }
		,{ DateTime({ 30,9,19, },{ 10,0 }),4 } 
	};

	constexpr R_Profile profiles_f[] = {
		//ProgID, ZnID, Days
		{ 0,0,100 } // At Home US MT--F--
		,{ 2,0,108 }// At Work US MT-TF--
		,{ 0,0,27 } // At Home US --WT-SS
		,{ 0,2,114 }// At Home DHW MTW--S-
		,{ 0,1,85}  // At Home DS M-W-F-S
		,{ 2,0,19 } // At Work US --W--SS
		,{ 0,1,42 } // At Home DS -T-T-S-
		,{ 0,2,13 } // At Home DHW ---TF-S

		,{ 4,0,255 } // Away US MTWTFSS
		,{ 3,2,255 } // Empty DHW 
		,{ 4,1,255 } // Away DS
		,{ 4,2,255 } // Away DHW 
		,{ 1,2,125 } // Occupied DHW MTWTF-S
		,{ 2,1,108 } // At Work DS MT-TF--
		,{ 2,1,19 }  // At Work DS --W--SS
		,{ 2,2,108 } // At Work DHW MT-TF--

		,{ 2,2,19 }  // At Work DHW --W--SS
		,{ 1,2,2 }   // Occupied DHW -----S-
		,{ 1,3,125}  // Occupied Flat MTWTF-S
		,{ 1,3,2 }   // Occupied Flat -----S- 
		,{ 3,3,255}  // Empty Flat 
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
		logger().log("setFactoryDefaults Started...");
		logger().log("\nThermalStore Table ");
		db.createTable(thermalStore_f);
		logger().log("\nMixValveContr Table ");
		db.createTable(mixValveControl_f);
		logger().log("\nDisplay Table ");
		db.createTable(displays_f);
		logger().log("\nRelays Table ");
		db.createTable(relays_f);
		logger().log("\nTempSensors Table ");
		db.createTable(tempSensors_f);
		logger().log("\nDwellings Table ");
		db.createTable(dwellings_f);
		logger().log("\nZones Table ");
		db.createTable(zones_f);
		logger().log("\nDwellingZones Table ");
		db.createTable(dwellingZones_f);
		logger().log("\nPrograms Table ");
		db.createTable(programs_f);
		logger().log("\nProfiles Table ");
		db.createTable(profiles_f);
		logger().log("\nTimeTemps Table ");
		db.createTable(timeTemps_f, i_09_orderedInsert);
		logger().log("\nSpells Table ");
		db.createTable(spells_f, i_09_orderedInsert);
		logger().log("\nAll Tables Created\n ");

		logger().log("setFactoryDefaults Completed");
	}
	
}
