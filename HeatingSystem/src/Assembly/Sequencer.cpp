#include "Sequencer.h"
#include "TemperatureController.h"
#include "Clock.h"
#include "Database.h"
#include "..\Client_DataStructures\Data_Spell.h"

#ifdef ZSIM
#include <iostream>
using namespace std;
#endif

using namespace RelationalDatabase;
using namespace client_data_structures;

namespace Assembly {
	using namespace Date_Time;

	Sequencer::Sequencer(Database & db, TemperatureController & tc) :
		_db(&db)
		, _tc(&tc)
		, _ttEndDateTime(clock_().now())
	{}

	auto Sequencer::getCurrentSpell(RecordID dwellingID, R_Spell & nextSpell) -> R_Spell {
		_db->_q_dwellingSpells.setMatchArg(dwellingID);
		Answer_R<R_Spell> prevSpell;
		for (Answer_R<R_Spell> spell : _db->_q_dwellingSpells) {
			//cout << spell.rec() << endl;
			if (spell.rec() > clock_().now()) {
				nextSpell = spell.rec();
				break;
			}
			prevSpell.deleteRecord();
			prevSpell = spell;
			nextSpell = spell.rec();
		}
		return prevSpell.rec();
	}

	auto Sequencer::getCurrentProfileID(RecordID zoneID, RecordID progID, RecordID & nextDayProfileID)->RecordID {
		_db->_q_zoneProfiles.setMatchArg(zoneID);
		_db->_q_profile.setMatchArg(progID);
		auto dayFlag = clock_().now().weekDayFlag();
		auto nextDayFlag = clock_().now().addDays(1).weekDayFlag();
		RecordID currProfileID;
		for (Answer_R<R_Profile> profile : _db->_q_profile) {
			if (profile.rec().days & dayFlag) {
				currProfileID = profile.id();
				logger().log("           Profile ID: ", profile.id(), " Days: ", profile.rec().days);
			}
			if (profile.rec().days & nextDayFlag) {
				nextDayProfileID = profile.id();
				logger().log("           Next Profile ID: ", profile.id(), " Days: ", profile.rec().days);
			}
		}
		return currProfileID;
	}

	auto Sequencer::getStartProfileID_for_Spell(R_Spell spell, RecordID zoneID, RecordID & prevDayProfileID)->RelationalDatabase::RecordID {
		_db->_q_zoneProfiles.setMatchArg(zoneID);
		_db->_q_profile.setMatchArg(spell.programID);
		auto startDayFlag = spell.date.weekDayFlag();
		auto prevDayFlag = clock_().now().addDays(-1).weekDayFlag();
		RecordID currProfileID;
		for (Answer_R<R_Profile> profile : _db->_q_profile) {
			if (profile.rec().days & startDayFlag) {
				currProfileID = profile.id();
				logger().log("           Profile ID: ", profile.id(), " Days: ", profile.rec().days);
			}
			if (profile.rec().days & prevDayFlag) {
				prevDayProfileID = profile.id();
				logger().log("           Prev Profile ID: ", profile.id(), " Days: ", profile.rec().days);
			}
		}
		return currProfileID;
	}
	
	auto Sequencer::getCurrentTT(RecordID profileID, R_TimeTemp & next_tt)->R_TimeTemp {
		_db->_q_timeTemps.setMatchArg(profileID);
		auto curr_tt = R_TimeTemp{};
		auto currTime = clock_().now().time();
		for (Answer_R<R_TimeTemp> timeTemp : _db->_q_timeTemps) {
			//cout << timeTemp.rec() << endl;
			if (timeTemp.rec().time() > currTime) {
				next_tt = timeTemp.rec();
				break;
			}
			curr_tt = timeTemp.rec();
			next_tt = curr_tt;
		}
		return curr_tt;
	}

	auto Sequencer::getTT_for_Time(TimeOnly time, RecordID profileID, RecordID prevDayProfileID)->R_TimeTemp {
		_db->_q_timeTemps.setMatchArg(profileID);
		auto curr_tt = R_TimeTemp{};
		for (Answer_R<R_TimeTemp> timeTemp : _db->_q_timeTemps) {
			//cout << timeTemp.rec() << endl;
			if (timeTemp.rec().time_temp.time() > time) {
				break;
			}
			curr_tt = timeTemp.rec();
		}
		if (curr_tt == R_TimeTemp{}) {
			_db->_q_timeTemps.setMatchArg(prevDayProfileID);
			Answer_R<R_TimeTemp> timeTemp = *_db->_q_timeTemps.last();
			//cout << timeTemp.rec() << endl;
			curr_tt = timeTemp.rec();
		}
		return curr_tt;
	}

	auto Sequencer::getFirstTT_for_profile(RecordID & profileID)->R_TimeTemp {
		_db->_q_timeTemps.setMatchArg(profileID);
		Answer_R<R_TimeTemp> timeTemp = *_db->_q_timeTemps.begin();
		//cout << timeTemp.rec() << endl;
		return timeTemp.rec();
	}

	void Sequencer::getNextEvent() {
		// Lambdas
		auto nextOccurrenceOfThisTime = [](TimeOnly time, DateTime date) mutable { 
			if (time < date.time()) {
				date.addDays(1);
			}
			date.time() = time;
			return date;
		};

		//return;
		if (clock_().now() >= _ttEndDateTime) {
			_ttEndDateTime = JUDGEMEMT_DAY;
			logger().log("Sequencer::getNextEvent()");
			//cout << " Current Date: " << clock_().now() << endl;

			for (Answer_R<R_Dwelling> dwelling : _db->_q_dwellings) {
				logger().log("Dwelling ", dwelling.id(), dwelling.rec().name);
				R_Spell nextSpell;
				auto currentProgID = getCurrentSpell(dwelling.id(), nextSpell).programID;
				logger().log("        Program ", currentProgID);
				//cout << " NextSpell at: " << nextSpell.date << endl;
				_db->_q_dwellingZones.setMatchArg(dwelling.id());
				for (Answer_R<R_Zone> zone : _db->_q_dwellingZones) {
					if (_tc->zoneArr[zone.id()].nextEventTime() <= clock_().now()) {
						logger().log("    Zone ", zone.id(), zone.rec().name);
						auto nextDayProfileID = RecordID{};
						auto profileID = getCurrentProfileID(zone.id(), currentProgID, nextDayProfileID);
						auto next_tt = R_TimeTemp{};
						auto currTT = getCurrentTT(profileID, next_tt); // next_tt will be the current TT if there isn't a later TT in this profile
						_tc->zoneArr[zone.id()].setProfileTempRequest(currTT.time_temp.temp());
						// GetNext
						if (next_tt == currTT) { // next_tt will be the current TT if there isn't a later TT in this profile
							next_tt = getFirstTT_for_profile(nextDayProfileID);
						}
						auto nextEvent = nextOccurrenceOfThisTime(next_tt.time(), clock_().now());
						//cout << " This Prog next TT Event at: " << nextEvent << endl;

						if (nextSpell.programID != currentProgID && nextSpell.date < nextEvent) {
							nextEvent = nextSpell.date;
							auto prevDayProfileID = RecordID{};
							profileID = getStartProfileID_for_Spell(nextSpell, zone.id(), prevDayProfileID);
							next_tt = getTT_for_Time(nextSpell.date, profileID, prevDayProfileID);
						}
						_tc->zoneArr[zone.id()].setNextEventTime(nextEvent);
						_tc->zoneArr[zone.id()].setNextProfileTempRequest(next_tt.time_temp.temp());
						if (nextEvent < _ttEndDateTime) _ttEndDateTime = nextEvent;
					}
				}
				//cout << endl;
			}
		}
	}

}
