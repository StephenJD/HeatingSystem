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
		, _q_displays{ rdb.tableQuery(TB_Display) }
		, _q_dwellings{ rdb.tableQuery(TB_Dwelling) }
		, _q_zones{ rdb.tableQuery(TB_Zone) }
		, _q_dwellingZones{ rdb.tableQuery(TB_DwellingZone), rdb.tableQuery(TB_Zone), 0, 1 }
		, _q_zoneDwellings{ rdb.tableQuery(TB_DwellingZone), rdb.tableQuery(TB_Dwelling), 1, 0 }
		, _q_dwellingProgs{ rdb.tableQuery(TB_Program), 1 }
		, _q_dwellingSpells{ rdb.tableQuery(TB_Spell), rdb.tableQuery(TB_Program), 1, 1 }
		, _q_spellProgs{ _q_dwellingSpells, rdb.tableQuery(TB_Program), 1 ,1 }
		, _q_spellProg{ rdb.tableQuery(TB_Spell), _q_spellProgs, 0 }
		, _q_progProfiles{ rdb.tableQuery(TB_Profile), 0 }
		, _q_zoneProfiles{ rdb.tableQuery(TB_Profile), 1 }
		, _q_profile{ _q_zoneProfiles, 0 }
		, _q_timeTemps{ rdb.tableQuery(TB_TimeTemp), 0 }
		, _q_tempSensors{ rdb.tableQuery(TB_TempSensor) }
		, _q_towelRail{ rdb.tableQuery(TB_TowelRail) }
		, _q_relay{ rdb.tableQuery(TB_Relay) }

		// DB Record Interfaces
		, _ds_currTime{ _recCurrTime , _recCurrTime.nullQuery }
		, _ds_dwellings{ _recDwelling, _q_dwellings }
		, _ds_zones{ _recZone, _q_zones }
		, _ds_dwZones{ _recZone, _q_dwellingZones, &_ds_dwellings }
		, _ds_dwProgs{ _recProg, _q_dwellingProgs, &_ds_dwellings }
		, _ds_dwSpells{ _recSpell, _q_dwellingSpells, &_ds_dwellings }
		, _ds_spellProg{ _recProg, _q_spellProg, &_ds_dwSpells }
		, _ds_profile{ _recProfile, _q_profile, &_ds_dwProgs, &_ds_dwZones }
		, _ds_timeTemps{ _recTimeTemp, _q_timeTemps, &_ds_profile }
		, _ds_tempSensors{ _recTempSensor, _q_tempSensors }
		, _ds_towelRail{ _recTowelRail, _q_towelRail}
		, _ds_relay{ _recRelay, _q_relay }
	{
		logger() << F("Database queries constructed") << L_endl;
	}

}
