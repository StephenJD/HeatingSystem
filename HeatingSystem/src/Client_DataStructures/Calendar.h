#pragma once
//#include "Date_Time.h"
//#include <RDB.h>
//
//namespace Client_DataStructures {
//	using namespace RelationalDatabase;
//	using namespace LCD_UI;
//	class Dataset_Spell;
//	/// A Spell is a Program together with a start date/Time and optionally a Duration.
//	/// A Spell without a Duration continues until the start of the next Spell without a Duration.
//	/// A Spell with a Duration temprarily overrides the previous Spell.
//	/// The period for which a Program is the active program is an Event.
//	/// An Event consists of an active Pragram with its start and end times.
//	/// The UI needs to present a Calendar of Events
//
//	class Calendar {
//	public:
//		Calendar(Dataset_Spell * spells);
//		struct nestedSpecials;
//		void loadSpellSequence();
//		void populateSequence(int firstSpell, int spellCount, int arrSize);
//		void removeNestedSpells(int arrSize);
//		void recycleSpell(int prevWkly, int t, nestedSpecials & nestSpec);
//		void removeOldSpells(int arrSize);
//		void sortSequence(int count);
//		int getNthSpell(ZdtData & zd, int position);
//		int findSpell(Date_Time::DateTime originalFromDate, Date_Time::DateTime originalToDate) const;
//		int currentWeeklySpell(ZdtData & zd, Date_Time::DateTime thisDate) const;
//	private:
//		enum { SPARE_SPELL = 127 };
//		struct startTime {
//			Date_Time::DateTime	start;
//			int8_t  spellID;
//			bool operator < (startTime rhs) { return start < rhs.start; }
//		};
//		struct nestedSpecials {
//			nestedSpecials();
//			int addSpell(int spellID, int & prevWkly); // returns removed DT index for possible recycling
//			int8_t spellArr[5];
//			uint8_t end;
//		};
//		startTime * _sequence;
//		Dataset_Spell * _spells = 0;
//	};
//}
