#include "Sequencer.h"
///#include "TemperatureController.h"
#include <Clock.h>
#include "HeatingSystem_Queries.h"
#include "..\Client_DataStructures\Data_Spell.h"

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

	Sequencer::Sequencer(HeatingSystem_Queries& queries) :
		_queries(&queries)
		{}

	ProfileInfo Sequencer::getProfileInfo(RecordID zoneID, DateTime timeOfInterest ) {
		ProfileInfo info;
		_queries->q_DwellingsForZone.setMatchArg(zoneID);
		for (Answer_R<R_Dwelling> dwelling : _queries->q_DwellingsForZone) {
			logger() << "\t" << dwelling.rec() << " " << timeOfInterest << L_endl;
			ProfileInfo dwellInfo;
			dwellInfo.currEvent = timeOfInterest;
			getCurrentSpell(dwelling.id(), dwellInfo);
			getCurrentProfileID(zoneID, dwellInfo);
#ifdef ZPSIM
			logger() << F("\t\tThis ") << dwellInfo.currTT
				<< F("\n\t\tNext ") << dwellInfo.nextTT << L_endl;
#endif
			if (info.nextEvent == Date_Time::JUDGEMEMT_DAY) info = dwellInfo;
			/*             _________
			*           ---|-----   |
			*           ¦  |     ¦--|-------    |
			* info______¦__|        |______¦____| ¦
			* dwell-----¦                  ¦------¦
			*         a b cd     e  f    g h
			* a) Higher CurrTT wins.
			* b) Event occurs whenever highest currentTT changes zone.
			*/
			if (dwellInfo.nextEvent < info.nextEvent) {
				//info.nextEvent = dwellInfo.nextEvent;
				/*a*/ if (dwellInfo.nextTT.time_temp.temp() > info.currTT.time_temp.temp()) {
					info.nextEvent = dwellInfo.nextEvent;
					info.nextTT = dwellInfo.nextTT;
				}
				/*f*/ else if (dwellInfo.currTT.time_temp.temp() > info.currTT.time_temp.temp() && dwellInfo.nextTT.time_temp.temp() <= info.currTT.time_temp.temp()) {
					info.nextEvent = dwellInfo.nextEvent;
					info.nextTT = info.currTT;
				}
				/*d*/ else if (info.nextTT.time_temp.temp() < dwellInfo.nextTT.time_temp.temp()) info.nextTT = dwellInfo.nextTT;
			}
			/*c*/  else if (info.nextTT.time_temp.temp() < dwellInfo.currTT.time_temp.temp() && info.nextEvent < dwellInfo.nextEvent) {
				info.nextTT = dwellInfo.currTT;
			}

			/*bdfh*/ if (dwellInfo.currTT.time_temp.temp() > info.currTT.time_temp.temp()) {
				info.currTT = dwellInfo.currTT;
			}			

		//ui_yield();
		}
		logger() << F("\tNext Event ") << info.nextEvent.getDayStr() << " " << info.nextEvent << L_endl;
		logger() << F("\t\tCurr ") << info.currTT
			<< F("\n\t\tNext ") << info.nextTT << L_endl;
		return info;
	}

	void Sequencer::getCurrentSpell(RecordID dwellingID, ProfileInfo& info) {
		bool deleteOld = (info.currEvent == clock_().now());
		_queries->q_SpellsForDwelling.setMatchArg(dwellingID);
		Answer_R<R_Spell> prevSpell{};
		for (Answer_R<R_Spell> spell : _queries->q_SpellsForDwelling) {
			if (spell.rec() > info.currEvent) {
				info.nextSpell = spell.rec();
				break;
			}
			if (deleteOld) prevSpell.deleteRecord();
			prevSpell = spell;
			info.nextSpell = spell.rec();
		}
		if (prevSpell.status() != TB_OK) prevSpell = *_queries->q_SpellsForDwelling.begin();
		info.currSpell = prevSpell.rec();
	}

	void Sequencer::getCurrentProfileID(RecordID zoneID, ProfileInfo & info) {
		/*
		* The next profile is always on the next day, but it might be from the next spell.
		* CurrSpell-NextDay 1stTT: 7:30
		* a) NextSp starts 10:00, 1stTT 11:00 - NextTT = CurrSp 7:30, then NextSp TT 11:00
		* b) NextSp starts 10:00, 1stTT 6:00  - NextTT = CurrSp 7:30, then NextSp TT 06:00
		* c) NextSp starts 6:00,  1stTT 11:00 - NextTT = CurrSp 7:30, then NextSp TT 11:00
		* d) NextSp starts 6:00,  1stTT 05:00 - NextTT = NextSp 5:00, then NextSp TT ...
		*  
		* The nextTT will be either:
		* a) The next TT after the current time in the next spell profile if it comes before the next TT in this spell
		* b) Otherwise: The next TT today in this profile
		* c) Or: The first TT in tomorrow's profile
		* 
		* If the next spell, 1stTT starts before this spell 1stTT, use next spell 1stTT
		* Otherwise use this spell next day 1st TT
		*/

		// Lambda
		auto getProfileForDay = [this, &info](RecordID program, int dayNo) -> RecordID {
			auto dayFlag = DateOnly::weekDayFlag(dayNo);
			auto prevDayFlag = DateOnly::weekDayFlag((dayNo+6)%7);
			auto nextDayFlag = DateOnly::weekDayFlag((dayNo+1)%7);
			
			_queries->q_ProfilesForZoneProg.setMatchArg(program);
			auto profileRS = _queries->q_ProfilesForZoneProg.begin();
			auto profileID = RecordID(-1);
			do {
				Answer_R<R_Profile> profile = *profileRS;
				if (profile.rec().days & dayFlag) {
					profileID = profile.id();
					if (info.currentProfileID == RecordID(-1)) info.currentProfileID = profileID;
				}
				if (info.prevProfileID == RecordID(-1) && profile.rec().days & prevDayFlag) {
					info.prevProfileID = profile.id();
				}				
				if (info.nextProfileID == RecordID(-1) && profile.rec().days & nextDayFlag) {
					info.nextProfileID = profile.id();
				}
				if (profileID != RecordID(-1) && info.nextProfileID != RecordID(-1) && info.prevProfileID != RecordID(-1)) break;
				++profileRS;
			} while (profileRS->status() == TB_OK);
			return profileID;
		};

		auto setNextEventFromTT = [](ProfileInfo& info) {
			auto nextTTDate = info.currEvent;
			nextTTDate.time() = info.nextTT.time();
			if (nextTTDate <= info.currEvent) {
				nextTTDate.addDays(1);
			}
			if (nextTTDate < info.nextEvent) info.nextEvent = nextTTDate;
		};

		auto lastTT = [this](int profile) -> R_TimeTemp {
			_queries->q_TimeTempsForProfile.setMatchArg(profile);
			Answer_R<R_TimeTemp> timeTemp = *_queries->q_TimeTempsForProfile.last();
			return timeTemp.rec();
		};

		auto getCurrentTT = [this, lastTT](ProfileInfo& info) {
			_queries->q_TimeTempsForProfile.setMatchArg(info.currentProfileID);
			auto curr_tt = R_TimeTemp{};
			for (Answer_R<R_TimeTemp> timeTemp : _queries->q_TimeTempsForProfile) {
				if (timeTemp.rec().time_temp.time() > info.currEvent.time()) {
					break;
				}
				curr_tt = timeTemp.rec();
			}
			if (curr_tt == R_TimeTemp{}) {
				curr_tt = lastTT(info.prevProfileID);
			}
			info.currTT = curr_tt;
		};

		auto resetProfileIDs = [](ProfileInfo& info) {
			info.currentProfileID = RecordID(-1);
			info.prevProfileID = RecordID(-1);
			info.nextProfileID = RecordID(-1);
		};

		// Algorithm
		_queries->q_ProfilesForZone.setMatchArg(zoneID);
		_queries->q_ProfilesForZoneProg.setMatchArg(info.currSpell.programID);
		getProfileForDay(info.currSpell.programID, info.currEvent.weekDayNo());
		getCurrentTT(info);
		// currTT is last TT for currentSpell Program <= currEvent
		info.nextTT = getNextTT(info.currentProfileID, info.currTT.time());
		if (info.nextTT == info.currTT)
			info.nextTT = getNextTT(info.nextProfileID, TimeOnly(0));
		setNextEventFromTT(info);
		// nextTT & nextEvent is nextTT for currentSpell Program
		if (info.nextSpell.date != info.currSpell.date && info.nextSpell.date <= info.nextEvent) {
			resetProfileIDs(info); // next spell starts before nextTT, so need to get prevTT for the new program
			auto nextSpellProfile = getProfileForDay(info.nextSpell.programID, info.nextSpell.date.weekDayNo());
			auto nextSpellFirstTT = getNextTT(nextSpellProfile, info.currTT.time());
			if (nextSpellFirstTT.time() < info.nextTT.time()) { // nextSpellProfile has earlierTT to use as nextTT
				info.nextProfileID = nextSpellProfile;
				info.nextTT = nextSpellFirstTT;
			} else { // need to use last in prevSpellProfile as nextTT
				info.nextTT = lastTT(info.prevProfileID);
			}
			info.nextEvent = info.nextSpell.date;
		}
	}

	auto Sequencer::getNextTT(RecordID currProfileID, TimeOnly time) -> R_TimeTemp {
		_queries->q_TimeTempsForProfile.setMatchArg(currProfileID);
		auto tt_RS = _queries->q_TimeTempsForProfile.begin();
		Answer_R<R_TimeTemp> timeTemp = *tt_RS;
		auto nextTT = timeTemp.rec();
		while (tt_RS.status() == TB_OK) {
			timeTemp = *tt_RS;
			if (timeTemp.rec().time_temp.time() > time) {
				nextTT = timeTemp.rec();
				break;
			}
			++tt_RS;
		}
		return nextTT;
	}
}
