#pragma once
#include "..\LCD_UI\UI_Cmd.h"

namespace Client_DataStructures {

	class InsertSpell_Cmd : public LCD_UI::UI_Cmd
	{
	public:
		InsertSpell_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour = LCD_UI::viewOneUpDn());
		LCD_UI::Collection_Hndl * select(LCD_UI::Collection_Hndl * from) override;
		Collection_Hndl * on_back() override;
		Collection_Hndl * on_select() override;
	private:
		Collection_Hndl & enableInsert(bool enable);
	};

	class InsertTimeTemp_Cmd : public LCD_UI::UI_Cmd
	{
	public:
		InsertTimeTemp_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour = LCD_UI::viewOneUpDn());
		Collection_Hndl * enableCmds(int enable);
		Collection_Hndl * deleteTT(int);
		Collection_Hndl * select(Collection_Hndl * from) override;
		bool back() override;
		Collection_Hndl * on_back() override;
		Collection_Hndl * on_select() override;
	private:
		enum { e_NewCmd, e_DelCmd, e_TTs };
		Collection_Hndl _tt_SubPage_h;
	};

}