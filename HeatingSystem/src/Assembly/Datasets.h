#pragma once
#include "HeatingSystem_Queries.h"

namespace Assembly {
	class TemperatureController;

	struct HeatingSystem_Datasets
	{
		HeatingSystem_Datasets(HeatingSystem_Queries& queries, TemperatureController& tc);
	private:
		// DB Record Interfaces - Must be constructed first
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
		client_data_structures::RecInt_MixValveController _recMixValve;
	public:
		// Datasets
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
		client_data_structures::Dataset _ds_mixValve;

	};

}

