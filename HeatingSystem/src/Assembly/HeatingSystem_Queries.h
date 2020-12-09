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
		client_data_structures::RecInt_Spell			_recSpell;
		client_data_structures::RecInt_Profile		_recProfile;
		client_data_structures::RecInt_TimeTemp		_recTimeTemp;
		client_data_structures::RecInt_TempSensor		_recTempSensor;
		client_data_structures::RecInt_TowelRail		_recTowelRail;
		client_data_structures::RecInt_Relay			_recRelay;

		// RDB Queries
		RelationalDatabase::TableQuery _q_displays;
		RelationalDatabase::TableQuery _q_dwellings;
		RelationalDatabase::TableQuery _q_zones;
		RelationalDatabase::QueryFL_T<client_data_structures::R_DwellingZone> _q_dwellingZones;
		RelationalDatabase::QueryFL_T<client_data_structures::R_DwellingZone> _q_zoneDwellings;
		RelationalDatabase::QueryF_T<client_data_structures::R_Program> _q_dwellingProgs;
		RelationalDatabase::QueryLF_T<client_data_structures::R_Spell, client_data_structures::R_Program> _q_dwellingSpells;
		RelationalDatabase::QueryLinkF_T<client_data_structures::R_Spell, client_data_structures::R_Program> _q_spellProgs;
		RelationalDatabase::QueryML_T<client_data_structures::R_Spell> _q_spellProg;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_progProfiles;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_zoneProfiles;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_profile;
		RelationalDatabase::QueryF_T<client_data_structures::R_TimeTemp> _q_timeTemps;
		RelationalDatabase::TableQuery _q_tempSensors;
		RelationalDatabase::TableQuery _q_towelRail;
		RelationalDatabase::TableQuery _q_relay;

		// DB Datasets
		client_data_structures::Dataset _ds_currTime;
		client_data_structures::Dataset _ds_dwellings;
		client_data_structures::Dataset _ds_zones;
		client_data_structures::Dataset _ds_dwZones;
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

