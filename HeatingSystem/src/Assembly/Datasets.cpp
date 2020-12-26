#include "Datasets.h"
#include "TemperatureController.h"

//using namespace client_data_structures;

namespace Assembly {

	HeatingSystem_Datasets::HeatingSystem_Datasets(HeatingSystem_Queries& queries, TemperatureController& tc) :
		// DB Record Interfaces
		  _recCurrTime()
		, _recDwelling()
		, _recZone(tc.zoneArr)
		, _recProg()
		, _recSpell()
		, _recProfile()
		, _recTimeTemp()
		, _recTempSensor(tc.tempSensorArr)
		, _recTowelRail(tc.towelRailArr)
		, _recRelay(tc.relayArr)

		// Datasets
		, _ds_currTime{ _recCurrTime , _recCurrTime.nullQuery }
		, _ds_dwellings{ _recDwelling, queries.q_Dwellings }
		, _ds_Zones{ _recZone, queries.q_Zones }
		, _ds_ZonesForDwelling{ _recZone, queries.q_ZonesForDwelling, &_ds_dwellings }
		, _ds_dwProgs{ _recProg, queries.q_ProgsForDwelling, &_ds_dwellings }
		, _ds_dwSpells{ _recSpell, queries.q_SpellsForDwelling, &_ds_dwellings }
		, _ds_spellProg{ _recProg, queries.q_ProgForSpell, &_ds_dwSpells }
		, _ds_profile{ _recProfile, queries.q_ProfilesForZoneProg, &_ds_dwProgs, &_ds_ZonesForDwelling }
		, _ds_timeTemps{ _recTimeTemp, queries.q_TimeTempsForProfile, &_ds_profile }
		, _ds_tempSensors{ _recTempSensor, queries.q_tempSensors }
		, _ds_towelRail{ _recTowelRail, queries.q_towelRails}
		, _ds_relay{ _recRelay, queries.q_relays }
	{
		logger() << F("Datasets constructed") << L_endl;
#ifdef ZPSIM
		ui_Objects()[(long)&_ds_ZonesForDwelling] = "_ds_ZonesForDwelling";
#endif
	}

}
