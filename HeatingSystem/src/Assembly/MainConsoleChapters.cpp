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
		, _currTimeUI_c {&db._rec_currTime, Dataset_WithoutQuery::e_currTime, Behaviour{V+S+Vn+UD_E+R0} }
		, _currDateUI_c{ &db._rec_currTime, Dataset_WithoutQuery::e_currDate, Behaviour{V+S+Vn+UD_E+R0} }
		, _dstUI_c{ &db._rec_currTime, Dataset_WithoutQuery::e_dst, Behaviour{V+S+V1+UD_E+R0} }
		, _SDCardUI_c{ &db._rec_currTime, Dataset_WithoutQuery::e_sdcard, {V + L0} }
		, _dwellNameUI_c { &db._rec_dwelling, Dataset_Dwelling::e_name }
		, _zoneIsReq_UI_c{ &db._rec_zones, Dataset_Zone::e_reqIsTemp, Behaviour{V+S+Vn+UD_E+LR+R0} }
		, _zoneNameUI_c{ &db._rec_dwZones, Dataset_Zone::e_name, {V + S + L + Vn + LR + UD_0 + R0} }
		, _zoneAbbrevUI_c{ &db._rec_dwZones, Dataset_Zone::e_abbrev}
		
		, _progNameUI_c{ &db._rec_dwProgs, Dataset_Program::e_name, Behaviour{V+S+Vn+LR+UD_0+R0} }
		, _dwellSpellUI_c{ &db._rec_dwSpells, Dataset_Spell::e_date, Behaviour{V + S + V1 + UD_E}, Behaviour{UD_E} }
		, _spellProgUI_c{ &db._rec_spellProg, Dataset_Program::e_name, Behaviour{V + S +L+ V1}, Behaviour{UD_A+R}}
		, _profileDaysUI_c{ &db._rec_profile, Dataset_ProfileDays::e_days, Behaviour{V+S+V1+UD_A+R}, Behaviour{UD_E + R}}
		
		, _timeTempUI_c{ &db._rec_timeTemps, Dataset_TimeTemp::e_TimeTemp, Behaviour{V + S +L+ Vn + LR+ UD_E + R0}, Behaviour{UD_E + R0}, 0,0, { static_cast<Collection_Hndl * (Collection_Hndl::*)(int)>(&InsertTimeTemp_Cmd::enableCmds), InsertTimeTemp_Cmd::e_allCmds } }
		, _tempSensorUI_c{ &db._rec_tempSensors, Dataset_TempSensor::e_name_temp, Behaviour{V + S + Vn + LR + R}}
		
		, _towelRailNameUI_c{ &db._rec_towelRailParent, Dataset_TowelRail::e_name }
		, _towelRailTempUI_c{ &db._rec_towelRailChild, Dataset_TowelRail::e_onTemp, Behaviour{V+S+V1}, Behaviour{UD_E} }
		, _towelRailOnTimeUI_c{ &db._rec_towelRailChild, Dataset_TowelRail::e_minutesOn, Behaviour{V + S + V1}, Behaviour{UD_E} }
		, _towelRailStatus_c{ &db._rec_towelRailChild, Dataset_TowelRail::e_secondsToGo, Behaviour{V + V1}, {} }
		
		, _relayStateUI_c{ &db._rec_relayParent, Dataset_Relay::e_state,{V + S + V1 + UD_E} }
		, _relayNameUI_c{ &db._rec_relayChild, Dataset_Relay::e_name, Behaviour{V+V1},{} }

		// Basic UI Elements
		, _dst{"DST Hours:"}
		, _prog{"Prg:", {V + L0} }
		, _zone{"Zne:"}
		, _backlightCmd{"Backlight",0, Behaviour{V + S +L + UD_A} }
		, _contrastCmd{"Contrast",0, Behaviour{V+S+UD_A} }
		, _dwellingZoneCmd{"Zones",0 }
		, _dwellingCalendarCmd{ "Calendar",0 }
		, _dwellingProgCmd{ "Programs",0 }
		, _insert{ "Insert-Prog", Behaviour{H + L0} }
		, _profileDaysCmd{ "Ds:",0}
		, _fromCmd{ "From", { &Collection_Hndl::move_focus_to,3 }, Behaviour{V+S+L+UD_A} }
		, _deleteTTCmd{ "Delete", 0, Behaviour{H+S+L+ LR} }
		, _editTTCmd{ "Edit", 0, Behaviour{H+S+ LR}}
		, _newTTCmd{ "New", 0, Behaviour{H + S } }
		, _towelRailsLbl{"Room Temp OnFor ToGo"}

		// Pages & sub-pages - Collections of UI handles
		, _page_currTime_c{ makeCollection(_currTimeUI_c, _SDCardUI_c, _currDateUI_c, _dst, _dstUI_c, _backlightCmd, _contrastCmd) }

		, _iterated_zoneReqTemp_c{80, _zoneIsReq_UI_c }

		, _calendar_subpage_c{ makeCollection(_dwellingCalendarCmd, _insert, _fromCmd, _dwellSpellUI_c, _spellProgUI_c) }
		, _iterated_prog_name_c{80, _progNameUI_c}
		, _prog_subpage_c{ makeCollection(_dwellingProgCmd, _iterated_prog_name_c) }

		, _iterated_zone_name_c{ 80, _zoneNameUI_c}
		, _zone_subpage_c{makeCollection(_dwellingZoneCmd, _iterated_zone_name_c)}

		, _page_dwellingMembers_subpage_c{ makeCollection(_calendar_subpage_c, _prog_subpage_c, _zone_subpage_c) }
		, _page_dwellingMembers_c{ makeCollection(_dwellNameUI_c, _page_dwellingMembers_subpage_c) }
		
		, _iterated_timeTempUI{80, _timeTempUI_c}
		, _tt_SubPage_c{ makeCollection(_deleteTTCmd, _editTTCmd, _newTTCmd, _iterated_timeTempUI) }
		, _page_profile_c{ makeCollection(_dwellNameUI_c, _prog, _progNameUI_c, _zone, _zoneAbbrevUI_c, _profileDaysCmd, _profileDaysUI_c, _tt_SubPage_c) }
		
		// Info Pages
		, _iterated_tempSensorUI{ 80, _tempSensorUI_c}
		
		, _iterated_towelRails_info_c{ 80, makeCollection(_towelRailNameUI_c,_towelRailTempUI_c, _towelRailOnTimeUI_c, _towelRailStatus_c),{V+S+Vn+UD_A+R} }
		, _page_towelRails_c{ makeCollection(_towelRailsLbl, _iterated_towelRails_info_c) }
		
		, _iterated_relays_info_c{80, makeCollection(_relayNameUI_c, _relayStateUI_c)}

		// Display - Collection of Page Handles
		, _user_chapter_c{ makeChapter(_page_currTime_c, _iterated_zoneReqTemp_c, _page_dwellingMembers_c, _page_profile_c) }
		, _user_chapter_h{_user_chapter_c}
		, _info_chapter_c{ makeChapter(_page_towelRails_c, _iterated_tempSensorUI, _iterated_relays_info_c) }
		, _info_chapter_h{_info_chapter_c}
	{
		_zone_subpage_c.behaviour().make_noRecycle();
		_calendar_subpage_c.behaviour().make_noRecycle();
		_prog_subpage_c.behaviour().make_noRecycle();
		_page_dwellingMembers_subpage_c.behaviour().make_viewOne();

		_backlightCmd.set_UpDn_Target(_backlightCmd.function(Contrast_Brightness_Cmd::e_backlight));
		_contrastCmd.set_UpDn_Target(_contrastCmd.function(Contrast_Brightness_Cmd::e_contrast));
		_dwellingZoneCmd.set_UpDn_Target(_page_dwellingMembers_c.item(1));
		_dwellingCalendarCmd.set_UpDn_Target(_page_dwellingMembers_c.item(1));
		_dwellingProgCmd.set_UpDn_Target(_page_dwellingMembers_c.item(1));
		_profileDaysCmd.set_UpDn_Target(_page_profile_c.item(6));
		_fromCmd.set_UpDn_Target(_calendar_subpage_c.item(3));
		_fromCmd.set_OnSelFn_TargetUI(_page_dwellingMembers_subpage_c.item(0));
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
		auto tt_Field_Interface_perittedVals = _timeTempUI_c.getStreamingTool().f_interface().editItem().get();
		ui_Objects()[(long)tt_Field_Interface_perittedVals] = "tt_PerittedVals";
		auto & tt_Field_Interface = _timeTempUI_c.getStreamingTool().f_interface();
		ui_Objects()[(long)&tt_Field_Interface] = "tt_Field_Interface";
		auto & zone_Field_Interface = _zoneAbbrevUI_c.getStreamingTool().f_interface();
		ui_Objects()[(long)&zone_Field_Interface] = "string_Interface";
		auto & profileDays_Field_Interface = _profileDaysUI_c.getStreamingTool().f_interface();
		ui_Objects()[(long)&profileDays_Field_Interface] = "profileDays_Field_Interface";
		
		ui_Objects()[(long)&_dwellNameUI_c] = "_dwellNameUI_c";
		ui_Objects()[(long)&_zoneIsReq_UI_c] = "_zoneIsReq_UI_c";
		ui_Objects()[(long)&_zoneAbbrevUI_c] = "_zoneAbbrevUI_c";
		ui_Objects()[(long)&_progNameUI_c] = "_progNameUI_c";
		ui_Objects()[(long)&_profileDaysUI_c] = "_profileDaysUI_c";

		ui_Objects()[(long)&_timeTempUI_c] = "_timeTempUI_c";
		ui_Objects()[(long)&_towelRailsLbl] = "_towelRailsLbl";
		ui_Objects()[(long)&_towelRailNameUI_c] = "_towelRailNameUI_c";
		ui_Objects()[(long)&_towelRailTempUI_c] = "_towelRailTempUI_c";
		ui_Objects()[(long)&_towelRailOnTimeUI_c] = "_towelRailOnTimeUI_c";
		ui_Objects()[(long)&_towelRailStatus_c] = "_towelRailStatus_c";
		ui_Objects()[(long)&_relayStateUI_c] = "_relayStateUI_c";
		ui_Objects()[(long)&_relayNameUI_c] = "_relayNameUI_c";

		ui_Objects()[(long)&_page_currTime_c] = "_page_currTime_c";
		ui_Objects()[(long)&_iterated_zoneReqTemp_c] = "_iterated_zoneReqTemp_c";
		ui_Objects()[(long)&_page_dwellingMembers_c] = "_page_dwellingMembers_c";
		ui_Objects()[(long)&_iterated_timeTempUI] = "_iterated_timeTempUI";
		ui_Objects()[(long)&_page_profile_c] = "_page_profile_c";

		ui_Objects()[(long)&_iterated_towelRails_info_c] = "_iterated_towelRails_info_c";
		ui_Objects()[(long)&_page_towelRails_c] = "_page_towelRails_c";

		ui_Objects()[(long)&_user_chapter_c] = "_user_chapter_c";
		ui_Objects()[(long)&_user_chapter_h] = "_user_chapter_h";
		ui_Objects()[(long)&_info_chapter_c] = "_info_chapter_c";
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
