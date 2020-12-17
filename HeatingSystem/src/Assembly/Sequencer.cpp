#include "Sequencer.h"
#include "TemperatureController.h"
#include <Clock.h>
#include "HeatingSystem_Queries.h"
#include "..\Client_DataStructures\Data_Spell.h"
#include "..\HardwareInterfaces\Zone.h"

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif

void ui_yield();

using namespace RelationalDatabase;
using namespace client_data_structures;
using namespace HardwareInterfaces;

namespace Assembly {
	using namespace Date_Time;

	Sequencer::Sequencer(HeatingSystem_Queries& queries, TemperatureController& tc) :
		_queries(&queries)
		, _tc(&tc)
		{}

	void Sequencer::recheckNextEvent() { // called when data has been edited by the user
		setNextEventTime(JUDGEMEMT_DAY);
		for (auto& zone : _tc->zoneArr) {
			zone.setNextEventTime(clock_().now()); // triggers re-evaluation of next event time
			refreshProfile(zone);
		}
	}

	void Sequencer::getNextEvent() { // called in every run of the arduino endless loop
		if (clock_().now() >= _nextSystemEvent) nextEvent();
	}

	void Sequencer::refreshProfile(Zone& zone) {
		// Lambdas
		auto nextOccurrenceOfThisTime = [](TimeOnly time, DateTime date) mutable {
			if (time <= date.time()) {
				date.addDays(1);
			}
			date.time() = time;
			return date;
		};

		auto getFirstTT_for_profile = [this](RecordID& profileID)->R_TimeTemp {
			queries()._q_TimeTempsForProfile.setMatchArg(profileID);
			Answer_R<R_TimeTemp> timeTemp = *queries()._q_TimeTempsForProfile.begin();
			//cout << timeTemp.rec() << endl;
			return timeTemp.rec();
		};

		auto currentTT_forThisProfile = [this](Answer_R<R_Zone>& zone, RecordID dwellingID, R_Spell& nextSpell, RecordID& currentProgID, RecordID& currentProfileID, RecordID& nextDayProfileID, R_TimeTemp& next_tt) {
			currentProgID = getCurrentSpell(dwellingID, nextSpell).programID;
			logger() << F("\tProgram ") << currentProgID << L_endl;
			currentProfileID = getCurrentProfileID(zone.id(), currentProgID, nextDayProfileID);
			logger() << F("\t\t\tThis profileID: ") << currentProfileID << F(" Next Day ProfileID: ") << nextDayProfileID;
			// ************ Actually, if now() < first TT of the day, then CurrentProfileID should be for the previous day.
			return getCurrentTT(currentProfileID, next_tt); // next_tt will be the current TT if there isn't a later TT in this profile
		};

		auto nextZoneEvent = zone.nextEventTime(); // zero on start-up
		logger() << "\t" << zone.zoneRecord().rec() << L_endl;
		if (clock_().now() >= nextZoneEvent) {
			nextZoneEvent = JUDGEMEMT_DAY;
			auto nextSpell = R_Spell{};
			auto currentProgID = RecordID{};
			auto currentProfileID = RecordID{};
			auto nextDayProfileID = RecordID{};
			auto next_tt = R_TimeTemp{};
			auto currentTempRequest = TimeTemp{}.temp();
			queries()._q_DwellingsForZone.setMatchArg(zone.id());
			for (Answer_R<R_Dwelling> dwelling : queries()._q_DwellingsForZone) {
				logger() << "\t\t" << dwelling.rec();
				auto currTT = currentTT_forThisProfile(zone.zoneRecord(), dwelling.id(), nextSpell, currentProgID, currentProfileID, nextDayProfileID, next_tt);
				logger() << F("\n\t\t\tThis ") << currTT
					<< F("\n\t\t\tNext ") << next_tt;
				// Highest current temperature request wins.
				if (currTT.time_temp.temp() > currentTempRequest) currentTempRequest = currTT.time_temp.temp();
				// GetNext TT
				if (next_tt == currTT) { // next_tt will be the current TT if there isn't a later TT in this profile
					next_tt = getFirstTT_for_profile(nextDayProfileID);
				}
				auto nextEvent = nextOccurrenceOfThisTime(next_tt.time(), clock_().now());
				logger() << F("\n\t\t\tThis Prog next ") << next_tt << F("\n\t\t\tEvent at: ") << nextEvent << L_endl;
				if (nextSpell.programID != currentProgID && nextSpell.date < nextEvent) {
					nextEvent = nextSpell.date;
					auto prevDayProfileID = RecordID{};
					currentProfileID = getStartProfileID_for_Spell(nextSpell, zone.id(), prevDayProfileID);
					next_tt = getTT_for_Time(nextSpell.date, currentProfileID, prevDayProfileID);
				}
				if (nextEvent < nextZoneEvent) {
					nextZoneEvent = nextEvent;
					// Earliest TT sets time and temp for next zone event.
					zone.setNextEventTime(nextZoneEvent);
					zone.setNextProfileTempRequest(next_tt.time_temp.temp());
				}
				ui_yield();
			}
			zone.setProfileTempRequest(currentTempRequest);
			logger() << F("\t\t\tCurrent Request for ") << zone.zoneRecord().rec().name << F(" is ") << zone.currTempRequest() << F("\tNext Request is ") << zone.nextTempRequest() << L_endl;
			logger() << F("\t\t\tNext ZoneEvent is at ") << zone.nextEventTime() << L_endl;
		} else {
			logger() << F("\t\tNot time for next event...\n");
		}
		if (nextZoneEvent < _nextSystemEvent) setNextEventTime(nextZoneEvent);
	}

	void Sequencer::nextEvent() {  // called when event has expired
		setNextEventTime(JUDGEMEMT_DAY);
		logger() << L_endl << L_time << F("Sequencer::nextEvent()\n");

		for (auto & zone : _tc->zoneArr) {
			refreshProfile(zone);
		}
		logger() << F("Next SystemEvent: ") << _nextSystemEvent << L_endl;
	}

	// Private functions called by nextEvent()

	auto Sequencer::getCurrentSpell(RecordID dwellingID, R_Spell& nextSpell) -> R_Spell {
		queries()._q_SpellsForDwelling.setMatchArg(dwellingID);
		Answer_R<R_Spell> prevSpell{};
		for (Answer_R<R_Spell> spell : queries()._q_SpellsForDwelling) {
			if (spell.rec() > clock_().now()) {
				nextSpell = spell.rec();
				break;
			}
			prevSpell.deleteRecord();
			prevSpell = spell;
			nextSpell = spell.rec();
		}
		if (prevSpell.status() != TB_OK) prevSpell = *queries()._q_SpellsForDwelling.begin();
		return prevSpell.rec();
	}

	auto Sequencer::getCurrentProfileID(RecordID zoneID, RecordID progID, RecordID& nextDayProfileID)->RecordID {
		queries()._q_ProfilesForZone.setMatchArg(zoneID);
		queries()._q_ProfilesForZoneProg.setMatchArg(progID);
		auto dayFlag = clock_().now().weekDayFlag();
		auto nextDayFlag = clock_().now().addDays(1).weekDayFlag();
		RecordID currProfileID = -1;
		for (Answer_R<R_Profile> profile : queries()._q_ProfilesForZoneProg) {
			if (profile.rec().days & dayFlag) {
				currProfileID = profile.id();
			}
			if (profile.rec().days & nextDayFlag) {
				nextDayProfileID = profile.id();
			}
		}
		return currProfileID;
	}

	auto Sequencer::getStartProfileID_for_Spell(R_Spell spell, RecordID zoneID, RecordID& prevDayProfileID)->RelationalDatabase::RecordID {
		queries()._q_ProfilesForZone.setMatchArg(zoneID);
		queries()._q_ProfilesForZoneProg.setMatchArg(spell.programID);
		auto startDayFlag = spell.date.weekDayFlag();
		auto prevDayFlag = clock_().now().addDays(-1).weekDayFlag();
		RecordID currProfileID = -1;
		for (Answer_R<R_Profile> profile : queries()._q_ProfilesForZoneProg) {
			if (profile.rec().days & startDayFlag) {
				currProfileID = profile.id();
			}
			if (profile.rec().days & prevDayFlag) {
				prevDayProfileID = profile.id();
			}
		}
		return currProfileID;
	}

	auto Sequencer::getCurrentTT(RecordID profileID, R_TimeTemp& next_tt)->R_TimeTemp {
		queries()._q_TimeTempsForProfile.setMatchArg(profileID);
		auto curr_tt = R_TimeTemp{};
		auto currTime = clock_().now().time();
		for (Answer_R<R_TimeTemp> timeTemp : queries()._q_TimeTempsForProfile) {
			//cout << timeTemp.rec() << endl;
			if (timeTemp.rec().time() > currTime) {
				next_tt = timeTemp.rec();
				if (curr_tt == R_TimeTemp{}) curr_tt = next_tt;
				break;
			}
			curr_tt = timeTemp.rec();
			next_tt = curr_tt;
		}
		return curr_tt;
	}

	auto Sequencer::getTT_for_Time(TimeOnly time, RecordID profileID, RecordID prevDayProfileID)->R_TimeTemp {
		queries()._q_TimeTempsForProfile.setMatchArg(profileID);
		auto curr_tt = R_TimeTemp{};
		for (Answer_R<R_TimeTemp> timeTemp : queries()._q_TimeTempsForProfile) {
			//cout << timeTemp.rec() << endl;
			if (timeTemp.rec().time_temp.time() > time) {
				break;
			}
			curr_tt = timeTemp.rec();
		}
		if (curr_tt == R_TimeTemp{}) {
			queries()._q_TimeTempsForProfile.setMatchArg(prevDayProfileID);
			Answer_R<R_TimeTemp> timeTemp = *queries()._q_TimeTempsForProfile.last();
			//cout << timeTemp.rec() << endl;
			curr_tt = timeTemp.rec();
		}
		return curr_tt;
	}
}
