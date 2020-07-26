#pragma once
#include "..\LCD_UI\A_Top_UI.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\LCD_UI\UI_Cmd.h"

namespace Assembly {
	class HeatingSystem_Queries;

	class RemoteConsoleChapters : public LCD_UI::Chapter_Generator
	{
	public:
		RemoteConsoleChapters(HeatingSystem_Queries & db);
		LCD_UI::A_Top_UI & operator()(int chapterNo) override;
	private:
		LCD_UI::UI_FieldData _rem_tempReqUI_c;
		LCD_UI::UI_FieldData _rem_tempIsUI_c;
		LCD_UI::UI_Label _rem_prompt;
		LCD_UI::UI_Label _rem_req_lbl;
		LCD_UI::UI_Label _rem_is_lbl;
		// Display - Collection of Page Handles
	public:
		LCD_UI::Collection<5, LCD_UI::Collection_Hndl> remotePage_c;
	private:
		LCD_UI::Collection<1, LCD_UI::Collection_Hndl> _remote_chapter_c;
		LCD_UI::A_Top_UI _remote_chapter_h;
	};
}
