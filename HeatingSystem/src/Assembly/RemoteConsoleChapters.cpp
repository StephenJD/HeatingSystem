#include "RemoteConsoleChapters.h"
#include "..\Client_DataStructures\Client_Cmd.h"
#include "HeatingSystem_Queries.h"

namespace Assembly {
	using namespace RelationalDatabase;
	using namespace client_data_structures;
	using namespace LCD_UI;

	RemoteConsoleChapters::RemoteConsoleChapters(HeatingSystem_Queries & db) :
		_rem_tempSensorUI_c{ &db._rec_zones, Dataset_Zone::e_reqIsTemp }
		, _rem_prompt{ "^v adjusts temp" }
		, _remotePage_c{makeCollection(_rem_prompt, _rem_tempSensorUI_c)}
		, _remote_chapter_c{makeCollection(_remotePage_c)}
		, _remote_chapter_h{ _remote_chapter_c }

	{
#ifdef ZPSIM
		ui_Objects()[(long)&_rem_tempSensorUI_c] = "_rem_tempSensorUI_c";
		ui_Objects()[(long)&_rem_prompt] = "_rem_prompt";
		ui_Objects()[(long)&_remotePage_c] = "_remotePage_c";
		ui_Objects()[(long)&_remote_chapter_c] = "_remote_chapter_c";
#endif
	}

	LCD_UI::A_Top_UI & RemoteConsoleChapters::operator()(int) {
		return _remote_chapter_h;
	}
}

