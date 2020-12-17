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
		// DB Record Interfaces
		client_data_structures::RecInt_CurrDateTime	_recCurrTime;
		client_data_structures::RecInt_Dwelling		_recDwelling;
		client_data_structures::RecInt_Zone			_recZone;
		client_data_structures::RecInt_Program		_recProg;
		client_data_structures::RecInt_Spell		_recSpell;
		client_data_structures::RecInt_Profile		_recProfile;
		client_data_structures::RecInt_TimeTemp		_recTimeTemp;
		client_data_structures::RecInt_TempSensor	_recTempSensor;
		client_data_structures::RecInt_TowelRail	_recTowelRail;
		client_data_structures::RecInt_Relay		_recRelay;

		// RDB Queries
		RelationalDatabase::TableQuery _q_Displays;
		RelationalDatabase::TableQuery _q_Dwellings;
		RelationalDatabase::TableQuery _q_Zones;
		RelationalDatabase::QueryFL_T<client_data_structures::R_DwellingZone> _q_ZonesForDwelling;
		RelationalDatabase::QueryFL_T<client_data_structures::R_DwellingZone> _q_DwellingsForZone;
		RelationalDatabase::QueryF_T<client_data_structures::R_Program> _q_ProgsForDwelling;
		RelationalDatabase::QueryLF_T<client_data_structures::R_Spell, client_data_structures::R_Program> _q_SpellsForDwelling;
		RelationalDatabase::QueryLinkF_T<client_data_structures::R_Spell, client_data_structures::R_Program> _q_ProgsForSpellDwelling;
		RelationalDatabase::QueryML_T<client_data_structures::R_Spell> _q_ProgForSpell;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_ProfilesForProg;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_ProfilesForZone;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_ProfilesForZoneProg;
		RelationalDatabase::QueryF_T<client_data_structures::R_TimeTemp> _q_TimeTempsForProfile;
		RelationalDatabase::TableQuery _q_tempSensors;
		RelationalDatabase::TableQuery _q_towelRail;
		RelationalDatabase::TableQuery _q_relay;

		// DB Datasets
		client_data_structures::Dataset _ds_currTime;
		client_data_structures::Dataset _ds_dwellings;
		client_data_structures::Dataset _ds_Zones;
		client_data_structures::Dataset _ds_ZonesForDwelling;
		client_data_structures::Dataset_Program _ds_dwProgs;
		client_data_structures::Dataset_Spell _ds_dwSpells;
		client_data_structures::Dataset_Program _ds_spellProg;
		client_data_structures::Dataset_Profile _ds_profile;
		client_data_structures::Dataset _ds_timeTemps;
		client_data_structures::Dataset _ds_tempSensors;
		client_data_structures::Dataset _ds_towelRail;
		client_data_structures::Dataset _ds_relay;
	};

}

