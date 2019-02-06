#include "Sequencer.h"
#include "TemperatureController.h"
#include "Clock.h"


namespace Assembly {
	using namespace Date_Time;

	Sequencer::Sequencer(RelationalDatabase::RDB<TB_NoOfTables> & db, TemperatureController & tc) :
		_db(&db)
		, _tc(&tc)
		, _ttEndDateTime(0)
	{}

	void Sequencer::getNextEvent() {
		if (clock_().now() > _ttEndDateTime) {
			_ttEndDateTime = clock_().now();
			_ttEndDateTime.addOffset({ m10,1 });
			_tc->zoneArr[Z_UpStairs].setProfileTempRequest(18);
			_tc->zoneArr[Z_DownStairs].setProfileTempRequest(20);
			_tc->zoneArr[Z_DHW].setProfileTempRequest(45);
			_tc->zoneArr[Z_Flat].setProfileTempRequest(20);
			for (auto & zone : _tc->zoneArr) {
				//zone.setProfileTempRequest(19);
				zone.setNextTempRequest(zone.currTempRequest());
				zone.setNextEventTime(_ttEndDateTime);
			}
		}
	}

}