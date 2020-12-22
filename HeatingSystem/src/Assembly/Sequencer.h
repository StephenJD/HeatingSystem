#pragma once
#include "RDB.h"
#include "Date_Time.h"
#include "..\Client_DataStructures\Data_Spell.h"
#include "..\Client_DataStructures\Data_TimeTemp.h"

namespace Assembly {
	struct HeatingSystem_Queries;

	struct ProfileInfo {
		Date_Time::DateTime currEvent;
		Date_Time::DateTime nextEvent = Date_Time::JUDGEMEMT_DAY;
		client_data_structures::R_Spell currSpell;
		client_data_structures::R_Spell nextSpell;
		RelationalDatabase::RecordID prevProfileID = -1;
		RelationalDatabase::RecordID currentProfileID = -1;
		RelationalDatabase::RecordID nextProfileID = -1;
		client_data_structures::R_TimeTemp currTT;
		client_data_structures::R_TimeTemp nextTT;
	};

	class Sequencer {
	public:
		Sequencer(HeatingSystem_Queries& queries);
		ProfileInfo getProfileInfo(RelationalDatabase::RecordID zoneID, Date_Time::DateTime timeOfInterest);
		//HeatingSystem_Queries& queries() { return *_queries; }
	private:
		void getCurrentSpell(RelationalDatabase::RecordID dwellingID, ProfileInfo& info);
		void getCurrentProfileID(RelationalDatabase::RecordID zoneID, ProfileInfo& info);
		auto getNextTT(RelationalDatabase::RecordID currProfileID, Date_Time::TimeOnly time) ->client_data_structures::R_TimeTemp;
		HeatingSystem_Queries* _queries = 0;
	};

}