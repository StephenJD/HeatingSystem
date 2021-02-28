#include "HeatingSystem_Queries.h"

using namespace client_data_structures;

namespace Assembly {

	HeatingSystem_Queries::HeatingSystem_Queries(RelationalDatabase::RDB<TB_NoOfTables>& rdb) :
		// RDB Queries
		  q_Displays{ rdb.tableQuery(TB_Display) }
		, q_Dwellings{ rdb.tableQuery(TB_Dwelling) }
		, q_ThermStore{ rdb.tableQuery(TB_ThermalStore) }
		, q_MixValve{ rdb.tableQuery(TB_MixValveContr) }
		, q_Zones{ rdb.tableQuery(TB_Zone) }
		, q_TimeTemps{ rdb.tableQuery(TB_TimeTemp) }
		, q_ZonesForDwelling{ rdb.tableQuery(TB_Zone), rdb.tableQuery(TB_DwellingZone), 0, 1 }
		, q_DwellingsForZone{ rdb.tableQuery(TB_Dwelling), rdb.tableQuery(TB_DwellingZone), 1, 0 }
		, q_ProgsForDwelling{ rdb.tableQuery(TB_Program), 1 }
		, q_SpellsForDwelling{ rdb.tableQuery(TB_Spell), rdb.tableQuery(TB_Program), 1, 1 }
		, q_ProgsForSpellDwelling{ rdb.tableQuery(TB_Program), q_SpellsForDwelling, 1 ,1 }
		, q_ProgForSpell{ q_ProgsForSpellDwelling, rdb.tableQuery(TB_Spell), 0 }
		, q_ProfilesForProg{ rdb.tableQuery(TB_Profile), 0 }
		, q_ProfilesForZone{ rdb.tableQuery(TB_Profile), 1 }
		, q_ProfilesForZoneProg{ q_ProfilesForZone, 0 }
		, q_TimeTempsForProfile{ q_TimeTemps, 0 }
		, q_tempSensors{ rdb.tableQuery(TB_TempSensor) }
		, q_towelRails{ rdb.tableQuery(TB_TowelRail) }
		, q_relays{ rdb.tableQuery(TB_Relay) }
		, q_mixValveControllers(rdb.tableQuery(TB_MixValveContr))
	{
		logger() << F("Database queries constructed") << L_endl;
	}

}
