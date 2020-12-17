#include "HeatingSystem_Queries.h"
#include "TemperatureController.h"

using namespace client_data_structures;

namespace Assembly {

	HeatingSystem_Queries::HeatingSystem_Queries(RelationalDatabase::RDB<TB_NoOfTables>& rdb, TemperatureController& tc) :
		_rdb(&rdb)
		// DB Record Interfaces
		, _recCurrTime()
		, _recDwelling()
		, _recZone(tc.zoneArr)
		, _recProg()
		, _recSpell()
		, _recProfile()
		, _recTimeTemp()
		, _recTempSensor(tc.tempSensorArr)
		, _recTowelRail(tc.towelRailArr)
		, _recRelay(tc.relayArr)

		// RDB Queries
		, _q_Displays{ rdb.tableQuery(TB_Display) }
		, _q_Dwellings{ rdb.tableQuery(TB_Dwelling) }
		, _q_Zones{ rdb.tableQuery(TB_Zone) }
		, _q_ZonesForDwelling{ rdb.tableQuery(TB_Zone), rdb.tableQuery(TB_DwellingZone), 0, 1 }
		, _q_DwellingsForZone{ rdb.tableQuery(TB_Dwelling), rdb.tableQuery(TB_DwellingZone), 1, 0 }
		, _q_ProgsForDwelling{ rdb.tableQuery(TB_Program), 1 }
		, _q_SpellsForDwelling{ rdb.tableQuery(TB_Spell), rdb.tableQuery(TB_Program), 1, 1 }
		, _q_ProgsForSpellDwelling{ rdb.tableQuery(TB_Program), _q_SpellsForDwelling, 1 ,1 }
		, _q_ProgForSpell{ _q_ProgsForSpellDwelling, rdb.tableQuery(TB_Spell), 0 }
		, _q_ProfilesForProg{ rdb.tableQuery(TB_Profile), 0 }
		, _q_ProfilesForZone{ rdb.tableQuery(TB_Profile), 1 }
		, _q_ProfilesForZoneProg{ _q_ProfilesForZone, 0 }
		, _q_TimeTempsForProfile{ rdb.tableQuery(TB_TimeTemp), 0 }
		, _q_tempSensors{ rdb.tableQuery(TB_TempSensor) }
		, _q_towelRail{ rdb.tableQuery(TB_TowelRail) }
		, _q_relay{ rdb.tableQuery(TB_Relay) }

		// DB Record Interfaces
		, _ds_currTime{ _recCurrTime , _recCurrTime.nullQuery }
		, _ds_dwellings{ _recDwelling, _q_Dwellings }
		, _ds_Zones{ _recZone, _q_Zones }
		, _ds_ZonesForDwelling{ _recZone, _q_ZonesForDwelling, &_ds_dwellings }
		, _ds_dwProgs{ _recProg, _q_ProgsForDwelling, &_ds_dwellings }
		, _ds_dwSpells{ _recSpell, _q_SpellsForDwelling, &_ds_dwellings }
		, _ds_spellProg{ _recProg, _q_ProgForSpell, &_ds_dwSpells }
		, _ds_profile{ _recProfile, _q_ProfilesForZoneProg, &_ds_dwProgs, &_ds_ZonesForDwelling }
		, _ds_timeTemps{ _recTimeTemp, _q_TimeTempsForProfile, &_ds_profile }
		, _ds_tempSensors{ _recTempSensor, _q_tempSensors }
		, _ds_towelRail{ _recTowelRail, _q_towelRail}
		, _ds_relay{ _recRelay, _q_relay }
	{
		logger() << F("Database queries constructed") << L_endl;
		ui_Objects()[(long)&_ds_ZonesForDwelling] = "_ds_ZonesForDwelling";
	}

}
