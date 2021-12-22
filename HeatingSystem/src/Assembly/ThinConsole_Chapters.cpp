#include "ThinConsole_Chapters.h"
#include "..\Client_DataStructures\Client_Cmd.h"
#include "Datasets.h"

namespace Assembly {
	using namespace RelationalDatabase;
	using namespace client_data_structures;
	using namespace LCD_UI;

	ThinConsole_Chapters::ThinConsole_Chapters(HeatingSystem_Datasets& db) :
		_rem_tempReqUI_c{ &db._ds_Zones, RecInt_Zone::e_remoteReqTemp, {V + S + V1 + UD_S} }
		, _rem_tempIsUI_c{ &db._ds_Zones, RecInt_Zone::e_isTemp, {V + L0} }
		, _rem_prompt{ "^v adjusts temp" }
		, _rem_req_lbl{ "Req" }
		, _rem_is_lbl{ "Is", {V+L0} }
		, remotePage_c{ 32, makeCollection(_rem_prompt, _rem_is_lbl, _rem_tempIsUI_c, _rem_req_lbl, _rem_tempReqUI_c ), {V+S+V1+UD_S} }
		, _remote_chapter_c(makeChapter(remotePage_c))
		, _remote_chapter_h( _remote_chapter_c )

	{
#ifdef ZPSIM
		ui_Objects()[&_rem_tempReqUI_c] = "_rem_tempReqUI_c";
		ui_Objects()[&_rem_tempIsUI_c] = "_rem_tempIsUI_c";
		ui_Objects()[&_rem_prompt] = "_rem_prompt";
		ui_Objects()[&_rem_req_lbl] = "_rem_req_lbl";
		ui_Objects()[&_rem_is_lbl] = "_rem_is_lbl";
		ui_Objects()[&remotePage_c] = "_remotePage_c";
		ui_Objects()[&_remote_chapter_c] = "_remote_chapter_c";
#endif
		_remote_chapter_h.selectPage();
		_remote_chapter_h.rec_select();
	}

	LCD_UI::A_Top_UI & ThinConsole_Chapters::operator()(int) {
		return _remote_chapter_h;
	}
}
