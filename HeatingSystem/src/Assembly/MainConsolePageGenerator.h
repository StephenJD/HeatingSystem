#pragma once
#include "HeatingSystemEnums.h"
#include "..\Client_DataStructures\Data_Dwelling.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_zone.h"
#include "..\Client_DataStructures\Data_Program.h"
#include "..\Client_DataStructures\Data_Spell.h"
#include "..\Client_DataStructures\Data_Profile.h"
#include "..\Client_DataStructures\Data_TimeTemp.h"
#include "..\Client_DataStructures\Data_CurrentTime.h"
#include "..\Client_DataStructures\Client_Cmd.h"

#include "..\LCD_UI\A_Top_UI.h"
#include "..\LCD_UI\UI_DisplayBuffer.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\LCD_UI\UI_Cmd.h"

#include <RDB.h>

namespace Assembly {

	class MainConsolePageGenerator
	{
	public:
		enum { e_zones, e_calendars, e_programs, e_towelRails, e_NO_OF_CMDS };
		MainConsolePageGenerator(RelationalDatabase::RDB<TB_NoOfTables> & db);
		LCD_UI::A_Top_UI & pages() { return _display_h; }
	
	private:
		RelationalDatabase::RDB<TB_NoOfTables> * _db;
		// Run-time data arrays
		client_data_structures::Zone _zoneArr[4] = { { 17,18 },{ 20,19 },{ 45,55 },{ 21,21 } };
		// RDB Queries
		RelationalDatabase::TableQuery _q_dwellings;
		RelationalDatabase::TableQuery _q_zones;
		RelationalDatabase::QueryL_T<client_data_structures::R_DwellingZone, client_data_structures::R_Zone> _q_dwellingZones;
		RelationalDatabase::QueryF_T<client_data_structures::R_Program> _q_dwellingProgs;
		RelationalDatabase::QueryLF_T<client_data_structures::R_Spell, client_data_structures::R_Program> _q_dwellingSpells;
		RelationalDatabase::QueryIJ_T<client_data_structures::R_Spell> _q_spellProg;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_progProfiles;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_zoneProfiles;
		RelationalDatabase::QueryF_T<client_data_structures::R_Profile> _q_profile;
		RelationalDatabase::QueryF_T<client_data_structures::R_TimeTemp> _q_timeTemps;
		
		// DB Record Interfaces
		client_data_structures::Dataset_WithoutQuery _rec_currTime;
		client_data_structures::Dataset_Dwelling _rec_dwelling;
		client_data_structures::Dataset_Zone _rec_zones;
		client_data_structures::Dataset_Zone _rec_dwZones;
		client_data_structures::Dataset_Program _rec_dwProgs;
		client_data_structures::Dataset_Spell _rec_dwSpells;
		client_data_structures::Dataset_Program _rec_spellProg;
		client_data_structures::Dataset_ProfileDays _rec_profile;
		client_data_structures::Dataset_TimeTemp _rec_timeTemps;


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
		LCD_UI::UI_ShortCollection _timeTempUI_sc;

		// Basic UI Elements
		LCD_UI::UI_Label _dst, _prog, _zone;
		LCD_UI::UI_Cmd _dwellingZoneCmd, _dwellingCalendarCmd, _dwellingProgCmd;
		LCD_UI::UI_Cmd _profileDaysCmd;
		client_data_structures::InsertSpell_Cmd _fromCmd;
		LCD_UI::UI_Label _insert;
		client_data_structures::InsertTimeTemp_Cmd _newTTCmd;
		client_data_structures::InsertTimeTemp_Cmd _deleteTTCmd;
		
		// Pages & sub-pages - Collections of UI handles
		LCD_UI::Collection<4, LCD_UI::Collection_Hndl> _page_currTime_c;
		LCD_UI::Collection<1, LCD_UI::Collection_Hndl> _page_zoneReqTemp_c;
		LCD_UI::Collection<2, LCD_UI::Collection_Hndl> _zone_subpage_c;
		LCD_UI::Collection<5, LCD_UI::Collection_Hndl> _calendar_subpage_c;
		LCD_UI::Collection<2, LCD_UI::Collection_Hndl> _prog_subpage_c;
		LCD_UI::Collection<3, LCD_UI::Collection_Hndl> _page_dwellingMembers_subpage_c;
		LCD_UI::Collection<3, LCD_UI::Collection_Hndl> _tt_SubPage_c;
		LCD_UI::Collection<2, LCD_UI::Collection_Hndl> _page_dwellingMembers_c;
		LCD_UI::Collection<8, LCD_UI::Collection_Hndl> _page_profile_c;

		// Display - Collection of Page Handles
		LCD_UI::Collection<4, LCD_UI::Collection_Hndl> _display_c;
		LCD_UI::A_Top_UI _display_h;
	};
}

