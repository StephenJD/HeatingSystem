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

		LCD_UI::UI_FieldData _dwellNameUI_c;
		LCD_UI::UI_FieldData _zoneIsReq_UI_c;
		LCD_UI::UI_FieldData _zoneNameUI_c;
		LCD_UI::UI_FieldData _zoneAbbrevUI_c;
		LCD_UI::UI_FieldData _progAllNameUI_c;
		LCD_UI::UI_FieldData _progNameUI_c;
		LCD_UI::UI_FieldData _dwellSpellUI_c;
		LCD_UI::UI_FieldData _spellProgUI_c;
		LCD_UI::UI_FieldData _profileDaysUI_c;
		LCD_UI::UI_FieldData _timeTempUI_c;
		LCD_UI::UI_FieldData _tempSensorUI_c;
		//LCD_UI::UI_FieldData _towelRailUI_c;
		LCD_UI::UI_ShortCollection _timeTempUI_sc;
		LCD_UI::UI_ShortCollection _tempSensorUI_sc;
		//LCD_UI::UI_ShortCollection _towelRailUI_sc;

		// Basic UI Elements
		LCD_UI::UI_Label _dst, _prog, _zone;
		client_data_structures::Contrast_Brightness_Cmd  _backlightCmd, _contrastCmd;
		LCD_UI::UI_Cmd _dwellingZoneCmd, _dwellingCalendarCmd, _dwellingProgCmd;
		LCD_UI::UI_Cmd _profileDaysCmd;
		client_data_structures::InsertSpell_Cmd _fromCmd;
		LCD_UI::UI_Label _insert;
		client_data_structures::InsertTimeTemp_Cmd _deleteTTCmd;
		client_data_structures::InsertTimeTemp_Cmd _editTTCmd;
		client_data_structures::InsertTimeTemp_Cmd _newTTCmd;
		LCD_UI::UI_Label _towelRailsLbl;
		
		// Pages & sub-pages - Collections of UI handles
		LCD_UI::Collection<6, LCD_UI::Collection_Hndl> _page_currTime_c;
		LCD_UI::Collection<1, LCD_UI::Collection_Hndl> _page_zoneReqTemp_c;
		LCD_UI::Collection<2, LCD_UI::Collection_Hndl> _zone_subpage_c;
		LCD_UI::Collection<5, LCD_UI::Collection_Hndl> _calendar_subpage_c;
		LCD_UI::Collection<2, LCD_UI::Collection_Hndl> _prog_subpage_c;
		LCD_UI::Collection<3, LCD_UI::Collection_Hndl> _page_dwellingMembers_subpage_c;
		LCD_UI::Collection<4, LCD_UI::Collection_Hndl> _tt_SubPage_c;
		LCD_UI::Collection<2, LCD_UI::Collection_Hndl> _page_dwellingMembers_c;
		LCD_UI::Collection<8, LCD_UI::Collection_Hndl> _page_profile_c;
		LCD_UI::Collection<1, LCD_UI::Collection_Hndl> _page_tempSensors_c;
		//LCD_UI::Collection<5, LCD_UI::Collection_Hndl> _page_towelRails_c;

		// Display - Collection of Page Handles
		LCD_UI::Collection<4, LCD_UI::Collection_Hndl> _user_chapter_c;
		LCD_UI::A_Top_UI _user_chapter_h;
		LCD_UI::Collection<1, LCD_UI::Collection_Hndl> _info_chapter_c;
		LCD_UI::A_Top_UI _info_chapter_h;
	};
}

