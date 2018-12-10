#include "MainConsolePageGenerator.h"
#include "..\HeatingSystem.h"


namespace Assembly {
	using namespace RelationalDatabase;
	using namespace client_data_structures;
	using namespace LCD_UI;

	MainConsolePageGenerator::MainConsolePageGenerator(RDB<TB_NoOfTables> & db) :
		_db(&db)
		// RDB Queries
		, _q_dwellings{ _db->tableQuery(TB_Dwelling) }
		, _q_zones{ _db->tableQuery(TB_Zone) }
		, _q_dwellingZones{ _db->tableQuery(TB_DwellingZone), _db->tableQuery(TB_Zone), 0, 1 }
		, _q_dwellingProgs{ _db->tableQuery(TB_Program), 1 }
		, _q_dwellingSpells{ _db->tableQuery(TB_Spell), _db->tableQuery(TB_Program), 1, 1 }
		, _q_spellProg{ _db->tableQuery(TB_Spell), _db->tableQuery(TB_Program),0 }
		, _q_progProfiles{ _db->tableQuery(TB_Profile), 0}
		, _q_zoneProfiles{ _db->tableQuery(TB_Profile), 1}
		, _q_profile{ _q_zoneProfiles, 0}
		, _q_timeTemps{ _db->tableQuery(TB_TimeTemp), 0 }

		// DB Record Interfaces
		, _rec_currTime{ Dataset_WithoutQuery() }
		, _rec_dwelling{ Dataset_Dwelling(_q_dwellings, noVolData, 0) }
		, _rec_zones{ _q_zones, _zoneArr, 0 }
		, _rec_dwZones{ _q_dwellingZones, _zoneArr, &_rec_dwelling }
		, _rec_dwProgs{ _q_dwellingProgs, noVolData, &_rec_dwelling }
		, _rec_dwSpells{ _q_dwellingSpells, noVolData, &_rec_dwelling }
		, _rec_spellProg{ _q_spellProg, noVolData, &_rec_dwSpells }
		, _rec_profile{ _q_profile, noVolData, &_rec_dwProgs, &_rec_dwZones }
		, _rec_timeTemps{ _q_timeTemps, noVolData, &_rec_profile }

		// DB UIs (Lazy-Collections)
		, _currTimeUI_c {&_rec_currTime, Dataset_WithoutQuery::e_currTime,0,0, editOnNextItem() }
		, _currDateUI_c{ &_rec_currTime, Dataset_WithoutQuery::e_currDate,0,0, editOnNextItem() }
		, _dstUI_c{ &_rec_currTime, Dataset_WithoutQuery::e_dst,0,0, editOnNextItem() }
		, _dwellNameUI_c { &_rec_dwelling, Dataset_Dwelling::e_name }
		, _zoneIsReq_UI_c{ &_rec_zones, Dataset_Zone::e_reqIsTemp,0,0, editOnNextItem().make_viewAll() }
		, _zoneNameUI_c{ &_rec_dwZones, Dataset_Zone::e_name,0,0, viewAllUpDn().make_newLine() }
		, _zoneAbbrevUI_c{ &_rec_dwZones, Dataset_Zone::e_abbrev,0,0, viewOneUpDnRecycle() }
		
		, _progAllNameUI_c{ &_rec_dwProgs, Dataset_Program::e_name,0,0, viewAllUpDn().make_newLine() }
		, _progNameUI_c{ &_rec_dwProgs, Dataset_Program::e_name,0,0, viewOneUpDnRecycle() }
		, _dwellSpellUI_c{ &_rec_dwSpells, Dataset_Spell::e_date,0,0, editOnNextItem(), editRecycle() }
		, _spellProgUI_c{ &_rec_dwProgs, Dataset_Program::e_name,&_rec_dwSpells,Dataset_Spell::e_progID, viewOneUpDnRecycle().make_newLine(), editRecycle().make_unEditable() }
		, _profileDaysUI_c{ &_rec_profile, Dataset_ProfileDays::e_days,0,0, viewOneUpDnRecycle(), editRecycle() }
		, _timeTempUI_c{ &_rec_timeTemps, Dataset_TimeTemp::e_TimeTemp,0,0, viewAll().make_newLine().make_editOnNext(), editNonRecycle(), { static_cast<Collection_Hndl * (Collection_Hndl::*)(int)>(&InsertTimeTemp_Cmd::enableCmds),1 } }
		, _timeTempUI_sc{ UI_ShortCollection{ 80, _timeTempUI_c } }

		// Basic UI Elements
		, _dst{"DST Hours:"}
		, _prog{"Prg:", viewable().make_sameLine() }
		, _zone{"Zne:"}
		, _dwellingZoneCmd{"Zones",0 }
		, _dwellingCalendarCmd{ "Calendar",0 }
		, _dwellingProgCmd{ "Programs",0 }
		, _profileDaysCmd{ "Ds:",0}
		, _fromCmd{ "From", { &Collection_Hndl::move_focus_to,3 }, viewOneUpDnRecycle().make_newLine() }
		, _insert{ "Insert-Event", hidden().make_sameLine() }
		, _newTTCmd{ "New", 0, viewOneUpDn().make_hidden().make_newLine() }
		, _deleteTTCmd{ "Delete", 0, viewOneUpDn().make_hidden() }

		// Pages & sub-pages - Collections of UI handles
		, _page_currTime_c{ makeCollection(_currTimeUI_c, _currDateUI_c, _dst, _dstUI_c) }
		, _page_zoneReqTemp_c{ makeCollection(_zoneIsReq_UI_c) }
		, _zone_subpage_c{ makeCollection(_dwellingZoneCmd, _zoneNameUI_c).set(viewAllUpDn())}
		, _calendar_subpage_c{ makeCollection(_dwellingCalendarCmd, _insert, _fromCmd, _dwellSpellUI_c, _spellProgUI_c).set(viewAllUpDn())  }
		, _prog_subpage_c{ makeCollection(_dwellingProgCmd, _progAllNameUI_c).set(viewAllUpDn()) }
		, _page_dwellingMembers_subpage_c{ makeCollection(_zone_subpage_c, _calendar_subpage_c, _prog_subpage_c).set(viewOneUpDnRecycle()) }
		, _tt_SubPage_c{ makeCollection(_newTTCmd, _deleteTTCmd, _timeTempUI_sc) }
		, _page_dwellingMembers_c{ makeCollection(_dwellNameUI_c, _page_dwellingMembers_subpage_c) }
		, _page_profile_c{ makeCollection(_dwellNameUI_c, _prog, _progNameUI_c, _zone, _zoneAbbrevUI_c, _profileDaysCmd, _profileDaysUI_c, _tt_SubPage_c) }

		// Display - Collection of Page Handles
		, _display_c{ makeDisplay(_page_currTime_c, _page_zoneReqTemp_c, _page_dwellingMembers_c, _page_profile_c) }
		, _display_h{_display_c}
	{
		_dwellingZoneCmd.set_UpDn_Target(_page_dwellingMembers_c.item(1));
		_dwellingCalendarCmd.set_UpDn_Target(_page_dwellingMembers_c.item(1));
		_dwellingProgCmd.set_UpDn_Target(_page_dwellingMembers_c.item(1));
		_profileDaysCmd.set_UpDn_Target(_page_profile_c.item(6));
		_fromCmd.set_UpDn_Target(_calendar_subpage_c.item(3));
		_fromCmd.set_OnSelFn_TargetUI(_page_dwellingMembers_subpage_c.item(1));
		_timeTempUI_c.set_OnSelFn_TargetUI(&_newTTCmd);
		_newTTCmd.set_OnSelFn_TargetUI(_page_profile_c.item(7));
		_deleteTTCmd.set_OnSelFn_TargetUI(&_newTTCmd);
		//UI_DisplayBuffer mainDisplayBuffer(mainDisplay);
		// Create infinite loop
		//display1_h.stream(mainDisplayBuffer);

	}

	//class DwellingCmdCollection : public LCD_UI::LazyCollection {
	//public:
	//	enum {e_zones, e_calendars, e_programs, e_towelRails, e_NO_OF_CMDS};
	//	using I_SafeCollection::item;
	//	DwellingCmdCollection() : LazyCollection(e_NO_OF_CMDS) {}
	//private:
	//	Collection_Hndl & item(int newIndex) override {
	//		if (newIndex == index() && object().get() != 0) return object();
	//		setObjectIndex(newIndex);
	//		switch (newIndex) {
	//		case e_zones:
	//		}
	//		return _dwellingCmd_h;
	//		//return swap(new ConstLabel(newIndex * 2));
	//	}
	//	Collection_Hndl _dwellingCmd_h;
	//	UI_Cmd _dwellingCmd;
	//};
}
