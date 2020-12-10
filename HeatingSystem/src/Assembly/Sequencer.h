#pragma once
#include "RDB.h"
#include "HeatingSystemEnums.h"
#include "Date_Time.h"

namespace client_data_structures {
	struct R_Spell;
	struct R_TimeTemp;
}

namespace HardwareInterfaces {
	class Zone;
}

namespace Assembly {
	class TemperatureController;
	class HeatingSystem_Queries;

	class Sequencer {
	public:
		Sequencer(HeatingSystem_Queries & queries, TemperatureController & tc);
		void recheckNextEvent(); // called when data has been edited by the user
		void getNextEvent(); // called in every run of the arduino endless loop
		void refreshProfile(HardwareInterfaces::Zone & zone);
		HeatingSystem_Queries & queries() {return *_queries;}
	private:
		void nextEvent(); // called when event has expired
		void setNextEventTime(Date_Time::DateTime nextEventTime) { _nextSystemEvent = nextEventTime; }
		auto getCurrentSpell(RelationalDatabase::RecordID dwellingID, client_data_structures::R_Spell & nextSpell)->client_data_structures::R_Spell;
		auto getCurrentProfileID(RelationalDatabase::RecordID zoneID, RelationalDatabase::RecordID progID, RelationalDatabase::RecordID & nextDayProfileID)->RelationalDatabase::RecordID;
		auto getStartProfileID_for_Spell(client_data_structures::R_Spell spell, RelationalDatabase::RecordID zoneID, RelationalDatabase::RecordID & prevDayProfileID)->RelationalDatabase::RecordID;
		auto getCurrentTT(RelationalDatabase::RecordID profileID, client_data_structures::R_TimeTemp & next_tt)->client_data_structures::R_TimeTemp;
		auto getTT_for_Time(Date_Time::TimeOnly time, RelationalDatabase::RecordID profileID, RelationalDatabase::RecordID prevDayProfileID)->client_data_structures::R_TimeTemp;

		HeatingSystem_Queries * _queries = 0;
		TemperatureController * _tc = 0;
		Date_Time::DateTime _nextSystemEvent{};
	};

}