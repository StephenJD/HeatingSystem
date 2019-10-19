#pragma once
#include "RDB.h"
#include "HeatingSystemEnums.h"
#include "Date_Time.h"

namespace client_data_structures {
	struct R_Spell;
	struct R_TimeTemp;
}

namespace Assembly {
	class TemperatureController;
	class Database;

	class Sequencer {
	public:
		Sequencer(Database & db, TemperatureController & tc);
		void getNextEvent();
	private:
		auto getCurrentSpell(RelationalDatabase::RecordID dwellingID, client_data_structures::R_Spell & nextSpell)->client_data_structures::R_Spell;
		auto getCurrentProfileID(RelationalDatabase::RecordID zoneID, RelationalDatabase::RecordID progID, RelationalDatabase::RecordID & nextDayProfileID)->RelationalDatabase::RecordID;
		auto getStartProfileID_for_Spell(client_data_structures::R_Spell spell, RelationalDatabase::RecordID zoneID, RelationalDatabase::RecordID & prevDayProfileID)->RelationalDatabase::RecordID;
		auto getCurrentTT(RelationalDatabase::RecordID profileID, client_data_structures::R_TimeTemp & next_tt)->client_data_structures::R_TimeTemp;
		auto getTT_for_Time(Date_Time::TimeOnly time, RelationalDatabase::RecordID profileID, RelationalDatabase::RecordID prevDayProfileID)->client_data_structures::R_TimeTemp;
		auto getFirstTT_for_profile(RelationalDatabase::RecordID & profileID)->client_data_structures::R_TimeTemp;

		Database * _db = 0;
		TemperatureController * _tc = 0;
		Date_Time::DateTime _ttEndDateTime;
	};

}