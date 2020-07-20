#include "HeatingSystem_Queries.h"
#include "TemperatureController.h"

using namespace client_data_structures;

namespace Assembly {

	HeatingSystem_Queries::HeatingSystem_Queries(RelationalDatabase::RDB<TB_NoOfTables> & rdb, TemperatureController & tc) :
		_rdb(&rdb)
		// RDB Queries
		, _q_displays{ rdb.tableQuery(TB_Display) }
		, _q_dwellings{ rdb.tableQuery(TB_Dwelling) }
		, _q_zones{ rdb.tableQuery(TB_Zone) }
		, _q_dwellingZones{ rdb.tableQuery(TB_DwellingZone), rdb.tableQuery(TB_Zone), 0, 1 }
		, _q_zoneDwellings{ rdb.tableQuery(TB_DwellingZone), rdb.tableQuery(TB_Dwelling), 1, 0 }
		, _q_dwellingProgs{ rdb.tableQuery(TB_Program), 1 }
		, _q_dwellingSpells{ rdb.tableQuery(TB_Spell), rdb.tableQuery(TB_Program), 1, 1 }
		, _q_spellProg{ rdb.tableQuery(TB_Spell), rdb.tableQuery(TB_Program),0 }
		, _q_progProfiles{ rdb.tableQuery(TB_Profile), 0 }
		, _q_zoneProfiles{ rdb.tableQuery(TB_Profile), 1 }
		, _q_profile{ _q_zoneProfiles, 0 }
		, _q_timeTemps{ rdb.tableQuery(TB_TimeTemp), 0 }
		, _q_tempSensors{ rdb.tableQuery(TB_TempSensor) }
		, _q_towelRails{ rdb.tableQuery(TB_TowelRail) }
		, _q_relays{ rdb.tableQuery(TB_Relay) }

		// DB Record Interfaces
		, _rec_currTime{ Dataset_WithoutQuery() }
		, _rec_dwelling{ Dataset_Dwelling(_q_dwellings, noVolData, 0) }
		, _rec_zones{ _q_zones, tc.zoneArr, 0 }
		, _rec_dwZones{ _q_dwellingZones, tc.zoneArr, &_rec_dwelling }
		, _rec_dwProgs{ _q_dwellingProgs, noVolData, &_rec_dwelling }
		, _rec_dwSpells{ _q_dwellingSpells, noVolData, &_rec_dwelling }
		, _rec_spellProg{ _q_spellProg, noVolData, &_rec_dwSpells }
		, _rec_profile{ _q_profile, noVolData, &_rec_dwProgs, &_rec_dwZones }
		, _rec_timeTemps{ _q_timeTemps, noVolData, &_rec_profile }
		, _rec_tempSensors{ _q_tempSensors, tc.tempSensorArr, 0 }
		, _rec_towelRails{ _q_towelRails, tc.towelRailArr, 0 }
		, _rec_relay{ _q_relays }
	{
		logger() << F("Database queries constructed") << L_endl;
	}

}
