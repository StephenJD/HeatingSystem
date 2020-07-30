#include "RemoteConsoleChapters.h"
#include "..\Client_DataStructures\Client_Cmd.h"
#include "HeatingSystem_Queries.h"

namespace Assembly {
	using namespace RelationalDatabase;
	using namespace client_data_structures;
	using namespace LCD_UI;

	RemoteConsoleChapters::RemoteConsoleChapters(HeatingSystem_Queries & db) :
		_rem_tempReqUI_c{ &db._rec_zones, Dataset_Zone::e_reqTemp,0,0, editOnNextItem() }
		, _rem_tempIsUI_c{ &db._rec_zones, Dataset_Zone::e_isTemp, &_rem_tempReqUI_c, 0, viewable() }
		, _rem_prompt{ "^v adjusts temp" }
		, _rem_req_lbl{ "Req" }
		, _rem_is_lbl{ "Is", viewable() }
		, remotePage_c{makeCollection(_rem_prompt, _rem_req_lbl, _rem_tempReqUI_c, _rem_is_lbl, _rem_tempIsUI_c)}
		, _remote_page_group_c{remotePage_c,viewOneSelectable()} // iteration sub, view-one
		, _remote_chapter_c{makeDisplay(_remote_page_group_c)}
		, _remote_chapter_h{ _remote_chapter_c }

	{
#ifdef ZPSIM
		ui_Objects()[(long)&_rem_tempReqUI_c] = "_rem_tempReqUI_c";
		ui_Objects()[(long)&_rem_tempIsUI_c] = "_rem_tempIsUI_c";
		ui_Objects()[(long)&_rem_prompt] = "_rem_prompt";
		ui_Objects()[(long)&_rem_req_lbl] = "_rem_req_lbl";
		ui_Objects()[(long)&_rem_is_lbl] = "_rem_is_lbl";
		ui_Objects()[(long)&remotePage_c] = "_remotePage_c";
		ui_Objects()[(long)&_remote_page_group_c] = "_remote_page_group_c";
		ui_Objects()[(long)&_remote_chapter_c] = "_remote_chapter_c";
#endif
		_remote_chapter_h.selectPage();
	}

	LCD_UI::A_Top_UI & RemoteConsoleChapters::operator()(int) {
		return _remote_chapter_h;
	}
}

