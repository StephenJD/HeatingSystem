#pragma once
#include "HeatingSystemEnums.h"
#include "..\HardwareInterfaces\Zone.h"
#include "..\LCD_UI\A_Top_UI.h"
#include "..\LCD_UI\UI_DisplayBuffer.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\LCD_UI\UI_Cmd.h"
#include "..\Client_DataStructures\Client_Cmd.h"

class HeatingSystem;

namespace HardwareInterfaces {
	class LocalDisplay;
}

namespace Assembly {
	class TemperatureController;
	class HeatingSystem_Queries;

	class MainConsoleChapters : public LCD_UI::Chapter_Generator
	{
	public:
		enum { e_zones, e_calendars, e_programs, e_towelRails, e_NO_OF_CMDS };
		MainConsoleChapters(HeatingSystem_Queries & db, TemperatureController & tc, HeatingSystem & hs);
		LCD_UI::A_Top_UI & operator()(int chapterNo) override;

	private:
		TemperatureController * _tc;

		// DB UIs (Lazy-Collections)
		LCD_UI::UI_FieldData _currTimeUI_c;
		LCD_UI::UI_FieldData _currDateUI_c;
		LCD_UI::UI_FieldData _dstUI_c;
		LCD_UI::UI_FieldData _SDCardUI_c;

		LCD_UI::UI_FieldData _dwellNameUI_c;
		LCD_UI::UI_FieldData _zoneNameUI_c;
		LCD_UI::UI_FieldData _zoneAbbrevUI_c;
		LCD_UI::UI_FieldData _allZoneAbbrevUI_c;
		LCD_UI::UI_FieldData _allZoneReqTemp_UI_c;
		LCD_UI::UI_FieldData _allZoneNames_UI_c;
		LCD_UI::UI_FieldData _allZoneIsTemp_UI_c;
		LCD_UI::UI_FieldData _allZoneIsHeating_UI_c;
		LCD_UI::UI_FieldData _zoneManAuto_c;
		LCD_UI::UI_FieldData _zoneRatio_c;
		LCD_UI::UI_FieldData _zoneTimeConst_c;

		LCD_UI::UI_FieldData _progNameUI_c;
		LCD_UI::UI_FieldData _dwellSpellUI_c;
		LCD_UI::UI_FieldData _spellProgUI_c;
		LCD_UI::UI_FieldData _profileDaysUI_c;
		
		LCD_UI::UI_FieldData _timeTempUI_c;
		
		LCD_UI::UI_FieldData _tempSensorNameUI_c;
		LCD_UI::UI_FieldData _tempSensorTempUI_c;
		
		LCD_UI::UI_FieldData _towelRailNameUI_c;
		LCD_UI::UI_FieldData _towelRailTempUI_c;
		LCD_UI::UI_FieldData _towelRailOnTimeUI_c;
		LCD_UI::UI_FieldData _towelRailStatus_c;
		LCD_UI::UI_FieldData _relayStateUI_c;
		LCD_UI::UI_FieldData _relayNameUI_c;

		// Basic UI Elements
		LCD_UI::UI_Label _newLine, _dst, _reqestTemp, _is, _prog, _zone;
		client_data_structures::Contrast_Brightness_Cmd  _backlightCmd, _contrastCmd;
		LCD_UI::UI_Cmd _dwellingZoneCmd, _dwellingCalendarCmd, _dwellingProgCmd;
		LCD_UI::UI_Label _insert;
		LCD_UI::UI_Cmd _profileDaysCmd;
		client_data_structures::InsertSpell_Cmd _fromCmd;
		client_data_structures::InsertTimeTemp_Cmd _deleteTTCmd;
		client_data_structures::InsertTimeTemp_Cmd _editTTCmd;
		client_data_structures::InsertTimeTemp_Cmd _newTTCmd;
		LCD_UI::UI_Label _towelRailsLbl;
		
		// Pages & sub-pages - Collections of UI handles
		LCD_UI::Collection<7> _page_currTime_c;
		LCD_UI::UI_IteratedCollection<6> _iterated_zoneReqTemp_c;
		LCD_UI::Collection<5> _calendar_subpage_c;
		LCD_UI::UI_IteratedCollection<1> _iterated_prog_name_c;
		LCD_UI::Collection<2> _prog_subpage_c;

		LCD_UI::UI_IteratedCollection<1> _iterated_zone_name_c;
		LCD_UI::Collection<3> _zone_subpage_c;

		LCD_UI::Collection<3> _page_dwellingMembers_subpage_c;
		LCD_UI::Collection<2> _page_dwellingMembers_c;

		LCD_UI::UI_IteratedCollection<1> _iterated_timeTempUI;
		LCD_UI::Collection<4>_tt_SubPage_c;
		LCD_UI::Collection<8> _page_profile_c;
		// Info pages
		LCD_UI::UI_IteratedCollection<2> _iterated_tempSensorUI;
	
		LCD_UI::UI_IteratedCollection<4> _iterated_towelRails_info_c;
		LCD_UI::Collection<2> _page_towelRails_c;
		
		LCD_UI::UI_IteratedCollection<2> _iterated_relays_info_c;
		LCD_UI::UI_IteratedCollection<4> _iterated_zoneSettings_info_c;

		// Display - Collection of Page Handles
		LCD_UI::Collection<4> _user_chapter_c;
		LCD_UI::A_Top_UI _user_chapter_h;
		LCD_UI::Collection<4> _info_chapter_c;
		LCD_UI::A_Top_UI _info_chapter_h;
	};
}

