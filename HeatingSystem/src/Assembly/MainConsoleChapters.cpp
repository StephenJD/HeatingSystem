#include "MainConsoleChapters.h"
#include "..\HeatingSystem.h"
#include "TemperatureController.h"
#include "HeatingSystem_Queries.h"

namespace Assembly {
	using namespace RelationalDatabase;
	using namespace client_data_structures;
	using namespace LCD_UI;

	MainConsoleChapters::MainConsoleChapters(HeatingSystem_Queries & db, TemperatureController & tc, HeatingSystem & hs) :
		_tc(&tc)

		// DB UIs (Lazy-Collections)
		, _currTimeUI_c {&db._rec_currTime, Dataset_WithoutQuery::e_currTime,0,0, editOnNextItem() }
		, _currDateUI_c{ &db._rec_currTime, Dataset_WithoutQuery::e_currDate,0,0, editOnNextItem() }
		, _dstUI_c{ &db._rec_currTime, Dataset_WithoutQuery::e_dst,0,0, editOnNextItem() }
		, _SDCardUI_c{ &db._rec_currTime, Dataset_WithoutQuery::e_sdcard,0,0, viewable()}
		, _dwellNameUI_c { &db._rec_dwelling, Dataset_Dwelling::e_name }
		, _zoneIsReq_UI_c{ &db._rec_zones, Dataset_Zone::e_reqIsTemp,0,0, editOnNextItem().make_viewAll() }
		, _zoneNameUI_c{ &db._rec_dwZones, Dataset_Zone::e_name,0,0, viewAllUpDn().make_newLine() }
		, _zoneAbbrevUI_c{ &db._rec_dwZones, Dataset_Zone::e_abbrev,0,0, viewOneUpDnRecycle() }
		
		, _progAllNameUI_c{ &db._rec_dwProgs, Dataset_Program::e_name,0,0, viewAllUpDn().make_newLine() }
		, _progNameUI_c{ &db._rec_dwProgs, Dataset_Program::e_name,0,0, viewOneUpDnRecycle() }
		, _dwellSpellUI_c{ &db._rec_dwSpells, Dataset_Spell::e_date,0,0, editOnNextItem(), editRecycle()}
		, _spellProgUI_c{ &db._rec_dwProgs, Dataset_Program::e_name,&_dwellSpellUI_c,Dataset_Spell::e_progID, viewOneUpDnRecycle().make_newLine(), editRecycle().make_unEditable() }
		, _profileDaysUI_c{ &db._rec_profile, Dataset_ProfileDays::e_days,0,0, viewOneUpDnRecycle(), editRecycle() }
		
		, _timeTempUI_c{ &db._rec_timeTemps, Dataset_TimeTemp::e_TimeTemp,0,0, viewAll().make_newLine().make_editOnNext(), editNonRecycle(), { static_cast<Collection_Hndl * (Collection_Hndl::*)(int)>(&InsertTimeTemp_Cmd::enableCmds), InsertTimeTemp_Cmd::e_allCmds } }
		, _timeTempUI_sc{ UI_ShortCollection{ 80, _timeTempUI_c } }
		
		, _tempSensorUI_c{ &db._rec_tempSensors, Dataset_TempSensor::e_name_temp,0,0, viewAll().make_newLine() }
		, _tempSensorUI_sc{ UI_ShortCollection{ 80, _tempSensorUI_c } }
		
		, _towelRailNameUI_c{ &db._rec_towelRails, Dataset_TowelRail::e_name,0,0, viewOneUpDnRecycle().make_newLine() }
		, _towelRailTempUI_c{ &db._rec_towelRails, Dataset_TowelRail::e_onTemp, &_towelRailNameUI_c,  Dataset_TowelRail::e_TowelRailID }
		, _towelRailOnTimeUI_c{ &db._rec_towelRails, Dataset_TowelRail::e_minutesOn, &_towelRailNameUI_c,  Dataset_TowelRail::e_TowelRailID }
		, _towelRailStatus_c{ &db._rec_towelRails, Dataset_TowelRail::e_secondsToGo, &_towelRailNameUI_c,  Dataset_TowelRail::e_TowelRailID }

		// Basic UI Elements
		, _dst{"DST Hours:"}
		, _prog{"Prg:", viewable().make_sameLine() }
		, _zone{"Zne:"}
		, _backlightCmd{"Backlight",0, viewOneUpDn().make_newLine() }
		, _contrastCmd{"Contrast",0, viewOneUpDn() }
		, _dwellingZoneCmd{"Zones",0 }
		, _dwellingCalendarCmd{ "Calendar",0 }
		, _dwellingProgCmd{ "Programs",0 }
		, _profileDaysCmd{ "Ds:",0}
		, _fromCmd{ "From", { &Collection_Hndl::move_focus_to,3 }, viewOneUpDnRecycle().make_newLine() }
		, _insert{ "Insert-Event", hidden().make_sameLine() }
		, _deleteTTCmd{ "Delete", 0, viewOneUpDn().make_hidden().make_newLine() }
		, _editTTCmd{ "Edit", 0, viewOneUpDn().make_hidden().make_viewAll() }
		, _newTTCmd{ "New", 0, viewOneUpDn().make_hidden() }
		, _towelRailsLbl{"Room Temp OnFor ToGo"}

		// Pages & sub-pages - Collections of UI handles
		, _page_currTime_c{ makeCollection(_currTimeUI_c, _SDCardUI_c, _currDateUI_c, _dst, _dstUI_c, _backlightCmd, _contrastCmd) }
		, _page_zoneReqTemp_c{ makeCollection(_zoneIsReq_UI_c) }
		, _zone_subpage_c{ makeCollection(_dwellingZoneCmd, _zoneNameUI_c).set(viewAllUpDn())}
		, _calendar_subpage_c{ makeCollection(_dwellingCalendarCmd, _insert, _fromCmd, _dwellSpellUI_c, _spellProgUI_c).set(viewAllUpDn())  }
		, _prog_subpage_c{ makeCollection(_dwellingProgCmd, _progAllNameUI_c).set(viewAllUpDn()) }
		, _page_dwellingMembers_subpage_c{ makeCollection(_calendar_subpage_c, _prog_subpage_c, _zone_subpage_c).set(viewOneUpDnRecycle()) }
		, _tt_SubPage_c{ makeCollection(_deleteTTCmd, _editTTCmd, _newTTCmd, _timeTempUI_sc) }
		, _page_dwellingMembers_c{ makeCollection(_dwellNameUI_c, _page_dwellingMembers_subpage_c) }
		, _page_profile_c{ makeCollection(_dwellNameUI_c, _prog, _progNameUI_c, _zone, _zoneAbbrevUI_c, _profileDaysCmd, _profileDaysUI_c, _tt_SubPage_c) }
		, _page_tempSensors_c{ makeCollection(_tempSensorUI_sc) }
		
		, _towelRails_info_c{ makeCollection(_towelRailNameUI_c,_towelRailTempUI_c, _towelRailOnTimeUI_c, _towelRailStatus_c).set(viewAllUpDn()) } // show all elements for one TR
		, _subpage_towelRails_c{ _towelRails_info_c} // show all TR's
		, _page_towelRails_c{ makeCollection(_towelRailsLbl, _subpage_towelRails_c) } // shows all fields for each TR

		// Display - Collection of Page Handles
		, _user_chapter_c{ makeDisplay(_page_currTime_c, _page_zoneReqTemp_c, _page_dwellingMembers_c, _page_profile_c) }
		, _user_chapter_h{_user_chapter_c}
		, _info_chapter_c{ makeDisplay(_page_towelRails_c, _page_tempSensors_c ) }
		, _info_chapter_h{_info_chapter_c}
	{
		_backlightCmd.set_UpDn_Target(_backlightCmd.function(Contrast_Brightness_Cmd::e_backlight));
		_contrastCmd.set_UpDn_Target(_contrastCmd.function(Contrast_Brightness_Cmd::e_contrast));
		_dwellingZoneCmd.set_UpDn_Target(_page_dwellingMembers_c.item(1));
		_dwellingCalendarCmd.set_UpDn_Target(_page_dwellingMembers_c.item(1));
		_dwellingProgCmd.set_UpDn_Target(_page_dwellingMembers_c.item(1));
		_profileDaysCmd.set_UpDn_Target(_page_profile_c.item(6));
		_fromCmd.set_UpDn_Target(_calendar_subpage_c.item(3));
		_fromCmd.set_OnSelFn_TargetUI(_page_dwellingMembers_subpage_c.item(1));
		_deleteTTCmd.set_OnSelFn_TargetUI(&_editTTCmd);
		_editTTCmd.set_OnSelFn_TargetUI(_page_profile_c.item(7));
		_newTTCmd.set_OnSelFn_TargetUI(&_editTTCmd);
		_timeTempUI_c.set_OnSelFn_TargetUI(&_editTTCmd);
		_contrastCmd.setDisplay(hs.mainDisplay);
		_backlightCmd.setDisplay(hs.mainDisplay);
		//_user_chapter_h.rec_select();
		//UI_DisplayBuffer mainDisplayBuffer(mainDisplay);
		// Create infinite loop
		//display1_h.stream(mainDisplayBuffer);
#ifdef ZPSIM
		ui_Objects()[(long)&_user_chapter_c] = "_user_chapter_c";
		ui_Objects()[(long)&_info_chapter_c] = "_info_chapter_c";
		ui_Objects()[(long)&_user_chapter_h] = "_user_chapter_h";
		ui_Objects()[(long)&_page_currTime_c] = "_page_currTime_c";
		ui_Objects()[(long)&_page_zoneReqTemp_c] = "_page_zoneReqTemp_c";
		ui_Objects()[(long)&_page_dwellingMembers_c] = "_page_dwellingMembers_c";
		ui_Objects()[(long)&_page_profile_c] = "_page_profile_c";
		ui_Objects()[(long)&_timeTempUI_sc] = "_timeTempUI_sc";
		ui_Objects()[(long)&_timeTempUI_c] = "_timeTempUI_c";
		ui_Objects()[(long)&_profileDaysUI_c] = "_profileDaysUI_c";
		ui_Objects()[(long)&_zoneAbbrevUI_c] = "_zoneAbbrevUI_c";
		ui_Objects()[(long)&_zoneIsReq_UI_c] = "_zoneIsReq_UI_c";
		ui_Objects()[(long)&_progNameUI_c] = "_progNameUI_c";
		ui_Objects()[(long)&_dwellNameUI_c] = "_dwellNameUI_c";
		ui_Objects()[(long)&_tt_SubPage_c] = "_tt_SubPage_c";
		ui_Objects()[(long)&_towelRails_info_c] = "_towelRails_info_c";
		ui_Objects()[(long)&_subpage_towelRails_c] = "_subpage_towelRails_c";
		ui_Objects()[(long)&_page_towelRails_c] = "_page_towelRails_c";
		ui_Objects()[(long)&_towelRailsLbl] = "_towelRailsLbl";
		ui_Objects()[(long)&_towelRailNameUI_c] = "_towelRailNameUI_c";
		ui_Objects()[(long)&_towelRailTempUI_c] = "_towelRailTempUI_c";
		ui_Objects()[(long)&_towelRailStatus_c] = "_towelRailStatus_c";
		auto tt_Field_Interface_perittedVals = _timeTempUI_c.getInterface().f_interface().editItem().get();
		ui_Objects()[(long)tt_Field_Interface_perittedVals] = "tt_PerittedVals";
		auto & tt_Field_Interface = _timeTempUI_c.getInterface().f_interface();
		ui_Objects()[(long)&tt_Field_Interface] = "tt_Field_Interface";
		auto & zone_Field_Interface = _zoneAbbrevUI_c.getInterface().f_interface();
		ui_Objects()[(long)&zone_Field_Interface] = "string_Interface";
		auto & profileDays_Field_Interface = _profileDaysUI_c.getInterface().f_interface();
		ui_Objects()[(long)&profileDays_Field_Interface] = "profileDays_Field_Interface";

#endif

	}

	LCD_UI::A_Top_UI & MainConsoleChapters::operator()(int chapterNo) { 
		switch (chapterNo) {
		case 0:	return _user_chapter_h;
		case 1: return _info_chapter_h;
		default: return _user_chapter_h;
		}
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
