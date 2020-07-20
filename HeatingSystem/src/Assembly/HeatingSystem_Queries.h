#pragma once
#include "HeatingSystemEnums.h"
#include "..\Client_DataStructures\Data_Dwelling.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_zone.h"
#include "..\Client_DataStructures\Data_Program.h"
#include "..\Client_DataStructures\Data_Spell.h"
#include "..\Client_DataStructures\Data_Profile.h"
#include "..\Client_DataStructures\Data_TimeTemp.h"
#include "..\Client_DataStructures\Data_TowelRail.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "..\Client_DataStructures\Data_CurrentTime.h"
#include "..\Client_DataStructures\Client_Cmd.h"
#include <RDB.h>

namespace Assembly {
	class TemperatureController;

	class HeatingSystem_Queries
	{
	public:
		HeatingSystem_Queries(RelationalDatabase::RDB<TB_NoOfTables> & rdb, TemperatureController & tc);

	//private:
		RelationalDatabase::RDB<TB_NoOfTables> * _rdb;
		// RDB Queries
		RelationalDatabase::TableQuery _q_displays;
		RelationalDatabase::TableQuery _q_dwellings;
		RelationalDatabase::TableQuery _q_zones;
		RelationalDatabase::QueryL_T<client_data_structures::R_DwellingZone, client_data_structures::R_Zone> _q_dwellingZones;
		RelationalDatabase::QueryL_T<client_data_structures::R_DwellingZone, client_data_structures::R_Dwelling> _q_zoneDwellings;
		RelationalDatabase::QueryF_T<client_data_structures::R_Program> _q_dwellingProgs;
		RelationalDatabase::QueryLF_T<client_data_structures::R_Spell, client_data_structures::R_Program> _q_dwellingSpells;
		RelationalDatabase::QueryIJ_T<client_data_structures::R_Spell> _q_spellProg;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_progProfiles;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_zoneProfiles;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_profile;
		RelationalDatabase::QueryF_T<client_data_structures::R_TimeTemp> _q_timeTemps;
		RelationalDatabase::TableQuery _q_tempSensors;
		RelationalDatabase::TableQuery _q_towelRails;
		RelationalDatabase::TableQuery _q_relays;

		// DB Record Interfaces
		client_data_structures::Dataset_WithoutQuery _rec_currTime;
		client_data_structures::Dataset_Dwelling _rec_dwelling;
		client_data_structures::Dataset_Zone _rec_zones;
		client_data_structures::Dataset_Zone _rec_dwZones;
		client_data_structures::Dataset_Program _rec_dwProgs;
		client_data_structures::Dataset_Spell _rec_dwSpells;
		client_data_structures::Dataset_Program _rec_spellProg;
		client_data_structures::Dataset_ProfileDays _rec_profile;
		client_data_structures::Dataset_TimeTemp _rec_timeTemps;
		client_data_structures::Dataset_TempSensor _rec_tempSensors;
		client_data_structures::Dataset_TowelRail _rec_towelRails;
		client_data_structures::Dataset_Relay _rec_relay;
	};

}

