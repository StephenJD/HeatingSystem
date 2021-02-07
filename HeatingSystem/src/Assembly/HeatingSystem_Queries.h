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
#include "..\Client_DataStructures\Data_MixValveControl.h"
#include "..\Client_DataStructures\Data_CurrentTime.h"
#include "..\Client_DataStructures\Client_Cmd.h"
#include <RDB.h>

namespace Assembly {

	struct HeatingSystem_Queries
	{
		HeatingSystem_Queries(RelationalDatabase::RDB<TB_NoOfTables> & rdb);

		//RelationalDatabase::RDB<TB_NoOfTables> * _rdb;

		// RDB Queries
		RelationalDatabase::TableQuery q_ThermStore;
		RelationalDatabase::TableQuery q_MixValve;
		RelationalDatabase::TableQuery q_Displays;
		RelationalDatabase::TableQuery q_Dwellings;
		RelationalDatabase::TableQuery q_Zones;
		RelationalDatabase::QueryFL_T<client_data_structures::R_DwellingZone> q_ZonesForDwelling;
		RelationalDatabase::QueryFL_T<client_data_structures::R_DwellingZone> q_DwellingsForZone;
		RelationalDatabase::QueryF_T<client_data_structures::R_Program> q_ProgsForDwelling;
		RelationalDatabase::QueryLF_T<client_data_structures::R_Spell, client_data_structures::R_Program> q_SpellsForDwelling;
		RelationalDatabase::QueryLinkF_T<client_data_structures::R_Program, client_data_structures::R_Spell> q_ProgsForSpellDwelling;
		RelationalDatabase::QueryML_T<client_data_structures::R_Spell> q_ProgForSpell;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> q_ProfilesForProg;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> q_ProfilesForZone;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> q_ProfilesForZoneProg;
		RelationalDatabase::QueryF_T<client_data_structures::R_TimeTemp> q_TimeTempsForProfile;
		RelationalDatabase::TableQuery q_tempSensors;
		RelationalDatabase::TableQuery q_towelRails;
		RelationalDatabase::TableQuery q_relays;
		RelationalDatabase::TableQuery q_mixValveControllers;
	};

}

