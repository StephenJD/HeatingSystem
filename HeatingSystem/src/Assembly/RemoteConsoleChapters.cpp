#include "RemoteConsoleChapters.h"
#include "..\Client_DataStructures\Client_Cmd.h"
#include "HeatingSystem_Queries.h"

namespace Assembly {
	using namespace RelationalDatabase;
	using namespace client_data_structures;
	using namespace LCD_UI;

	RemoteConsoleChapters::RemoteConsoleChapters(HeatingSystem_Queries & db) :
		_rem_tempReqUI_c{ &db._ds_zones, RecInt_Zone::e_reqTemp, {V + S + V1 + UD_S} }
		, _rem_tempIsUI_c{ &db._ds_zone_child, RecInt_Zone::e_isTemp, {V + L0} }
		, _rem_prompt{ "^v adjusts temp" }
		, _rem_req_lbl{ "Req" }
		, _rem_is_lbl{ "Is", {V+L0} }
		, remotePage_c{ 32, makeCollection(_rem_prompt, _rem_req_lbl, _rem_tempReqUI_c, _rem_is_lbl, _rem_tempIsUI_c), {V+S+V1+UD_S} }
		, _remote_chapter_c(makeChapter(remotePage_c))
		, _remote_chapter_h( _remote_chapter_c )

	{
#ifdef ZPSIM
		ui_Objects()[(long)&_rem_tempReqUI_c] = "_rem_tempReqUI_c";
		ui_Objects()[(long)&_rem_tempIsUI_c] = "_rem_tempIsUI_c";
		ui_Objects()[(long)&_rem_prompt] = "_rem_prompt";
		ui_Objects()[(long)&_rem_req_lbl] = "_rem_req_lbl";
		ui_Objects()[(long)&_rem_is_lbl] = "_rem_is_lbl";
		ui_Objects()[(long)&remotePage_c] = "_remotePage_c";
		ui_Objects()[(long)&_remote_chapter_c] = "_remote_chapter_c";
#endif
		_remote_chapter_h.selectPage();
		_remote_chapter_h.rec_select();
	}

	LCD_UI::A_Top_UI & RemoteConsoleChapters::operator()(int) {
		return _remote_chapter_h;
	}
}

