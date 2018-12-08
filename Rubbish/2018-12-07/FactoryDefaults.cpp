#include "FactoryDefaults.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Program.h"
#include "..\Client_DataStructures\Data_TimeTemp.h"
#include "..\Client_DataStructures\Data_Spell.h"
#include "..\Client_DataStructures\Data_Profile.h"
#include "..\Client_DataStructures\Data_Zone.h"

#include <ostream>

namespace Assembly {
	using namespace Client_DataStructures;

	constexpr R_Relay relays[] = {
		{ "Flat",6 << 1 }
		,{ "FlTR",1 << 1 }
		,{ "HsTR",0 << 1 }
		,{ "UpSt",5 << 1 }
		,{ "MFSt",2 << 1 }
		,{ "Gas",3 << 1 }
		,{ "DnSt",4 << 1 }
	};

	constexpr R_TempSensor tempSensors[] = {
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

	constexpr R_Dwelling dwellings[] = {
		{ "House" }
		,{ "HolAppt" }
	};

	constexpr R_Program programs[] = {
		{ "At Home",0 }
		,{ "Occup'd",1 }
		,{ "At Work",0 }
		,{ "Empty",1 }
		,{ "Away",0 }
	};
	//constexpr R_Program programs[] = {
	//	{ "At Home",0 }
	//	,{ "At Work",0 }
	//	,{ "Away",0 }
	//	,{ "Occupied",1 }
	//	,{ "Empty",1 }
	//};

	constexpr R_DwellingZone dwellingZones[] = {
		{ 1,3 }
		,{ 0,0 }
		,{ 0,1 }
		,{ 1,2 } 
		,{ 0,2 }
	};

	constexpr uint16_t makeTT(int hrs, int mins, int temp) {
		auto timeOnly = TimeOnly{ hrs,mins }.asInt() << 8;
		return timeOnly + temp + 10;
	}

	R_TimeTemp timeTemps[] = { // profileID,TT
		{0, makeTT(7,30,15)}
		,{0, makeTT(22,30,18)}
		,{1, makeTT(7,30,15)}
		,{1, makeTT(22,30,18)}
		,{2, makeTT(8,30,15)}
		,{2, makeTT(22,30,18)}
		,{3, makeTT(6,30,45)}
		,{3, makeTT(10,00,30)}
		,{3, makeTT(16,00,45)}
		,{3, makeTT(23,00,30)}
	};

	constexpr R_Spell spells[] = {
		{ DateTime({ 31,7,19, },{ 15,20 }),0 }
		,{ DateTime({ 12,9,19, },{ 7,30 }),1 }
		,{ DateTime({ 3,9,19, },{ 17,30 }),2 }
		,{ DateTime({ 5,9,19, },{ 10,0 }),3 }
		,{ DateTime({ 22,9,19, },{ 15,0 }),2 }
		,{ DateTime({ 30,9,19, },{ 10,0 }),4 } 
	};

	constexpr R_Profile profiles[] = {
		//ProgID, ZnID, Days
		{ 0,0,100 } // At Home US MT--F--
		,{ 2,0,108 } // At Work US MT-TF--
		,{ 0,0,27 } // US --WT-SS
		,{ 0,2,114 }// DHW MTW--S-
		,{ 0,1,85}  // DS M-W-F-S
		,{ 2,0,19 } // US --W--SS
		,{ 0,1,42 } // DS -T-T-S-
		,{ 0,2,13 } // DHW ---TF-S

		,{ 4,0,255 } // Away MTWTFSS
		,{ 3,2,255 } // Empty
		,{ 4,1,255 }
		,{ 4,2,255 }
		,{ 1,2,125 } // Occupied MTWTF-S

		,{ 2,1,108 }
		,{ 2,1,19 }
		,{ 2,2,108 }
		,{ 2,2,19 }


		,{ 1,2,2 }   // -----S-
		,{ 1,3,125}   
		,{ 1,3,2 }   
		,{ 3,3,255} 
	};	
		
	//constexpr R_Profile profiles[] = {
	//	//ProgID, ZnID, Days
	//	{ 0,0,100 } // At Home US MT--F--
	//	,{ 0,0,27 } // US --WT-SS
	//	,{ 0,1,85}  // DS M-W-F-S
	//	,{ 0,1,42 } // DS -T-T-S-
	//	,{ 0,2,114 }// DHW MTW--S-
	//	,{ 0,2,13 } // DHW ---TF-S
	//	,{ 2,0,108 } // At Work US MT-TF--
	//	,{ 2,0,19 } // US --W--SS

	//	,{ 2,1,108 }
	//	,{ 2,1,19 }
	//	,{ 2,2,108 }
	//	,{ 2,2,19 }

	//	,{ 4,0,255 } // Away MTWTFSS
	//	,{ 4,1,255 }
	//	,{ 4,2,255 }
	//	,{ 1,2,125 } // Occupied MTWTF-S

	//	,{ 1,2,2 }   // -----S-
	//	,{ 1,3,125}   
	//	,{ 1,3,2 }   
	//	,{ 3,2,255 } // Empty
	//	,{ 3,3,255} 
	//};	
	
	//constexpr R_Profile profiles[] = {
	//	//ProgID, ZnID, Days
	//	{ 0,0,100 } // At Home US MT--F--
	//	,{ 0,0,27 } // US --WT-SS
	//	,{ 0,1,85}  // DS M-W-F-S
	//	,{ 0,1,42 } // DS -T-T-S-
	//	,{ 0,2,114 }// DHW MTW--S-
	//	,{ 0,2,13 } // DHW ---TF-S
	//	,{ 1,0,108 } // At Work US MT-TF--
	//	,{ 1,0,19 } // US --W--SS

	//	,{ 1,1,108 }
	//	,{ 1,1,19 }
	//	,{ 1,2,108 }
	//	,{ 1,2,19 }

	//	,{ 2,0,255 } // Away MTWTFSS
	//	,{ 2,1,255 }
	//	,{ 2,2,255 }
	//	,{ 3,2,125 } // Occupied MTWTF-S

	//	,{ 3,2,2 }   // -----S-
	//	,{ 3,3,125}   
	//	,{ 3,3,2 }   
	//	,{ 4,2,255 } // Empty
	//	,{ 4,3,255} 
	//};

	constexpr R_Zone zones[] = { 
		{ "UpStrs","US",1,1,1,0,25,12,1,60 }
		,{ "DnStrs","DS",1,1,1,0,25,12,1,60 }
		,{ "DHW","DHW",1,1,1,0,60,12,1,3 }
		,{ "Flat","Flt",1,1,1,0,25,12,1,60 } 
	};

	void setFactoryDefaults(RDB<TB_NoOfTables> & db) {
		//enum tableIndex { TB_Relay, TB_TempSensor, TB_Dwelling, TB_Program, TB_DwellingZone, TB_TimeTemp, TB_Spell, TB_Profile, TB_Zone, TB_NoOfTables };

		//Serial.println("setFactoryDefaults Started");
		std::cout << "\nRelays Table ";
		db.createTable(relays);
		std::cout << "\nTempSensors Table ";
		db.createTable(tempSensors);
		std::cout << "\nDwellings Table ";
		db.createTable(dwellings);
		std::cout << "\nZones Table ";
		db.createTable(zones);
		std::cout << "\nDwellingZones Table ";
		db.createTable(dwellingZones);
		std::cout << "\nPrograms Table ";
		db.createTable(programs);
		std::cout << "\nProfiles Table ";
		db.createTable(profiles);
		std::cout << "\nTimeTemps Table ";
		db.createTable(timeTemps, i_09_orderedInsert);
		std::cout << "\nSpells Table ";
		db.createTable(spells, i_09_orderedInsert);
		std::cout << "\nAll Tables Created\n ";

		//Serial.println("setFactoryDefaults Completed");
	}
	
}
