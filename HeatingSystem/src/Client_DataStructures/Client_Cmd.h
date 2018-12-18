#pragma once
#include "..\LCD_UI\UI_Cmd.h"

namespace client_data_structures {

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
		enum TT_Cmds{ e_DelCmd, e_EditCmd, e_NewCmd, e_TTs , e_allCmds = 7, e_none};
		InsertTimeTemp_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour = LCD_UI::viewOneUpDn());
		Collection_Hndl * enableCmds(int cmd_to_show);
		void focusHasChanged(bool hasFocus) override ;
		Collection_Hndl * select(Collection_Hndl * from) override;
		bool back() override;
		Collection_Hndl * on_back() override;
		Collection_Hndl * on_select() override;
	private:
		Collection_Hndl _tt_SubPage_h;
		bool _hasInsertedNew = false;
	};

}