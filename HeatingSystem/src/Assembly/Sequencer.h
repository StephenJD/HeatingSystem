#pragma once
#include "RDB.h"
#include "HeatingSystemEnums.h"
#include "Date_Time.h"

namespace Assembly {
	class TemperatureController;

	class Sequencer {
	public:
		Sequencer(RelationalDatabase::RDB<TB_NoOfTables> & db, TemperatureController & tc);
		void getNextEvent();
	private:
		RelationalDatabase::RDB<TB_NoOfTables> * _db = 0;
		TemperatureController * _tc = 0;
		Date_Time::DateTime _ttEndDateTime;
	};

}