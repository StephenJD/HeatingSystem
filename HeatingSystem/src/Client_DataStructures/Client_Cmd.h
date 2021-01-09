#pragma once
#include "..\LCD_UI\UI_Cmd.h"

namespace HardwareInterfaces {
	class LocalDisplay;
}

namespace client_data_structures {

	class InsertSpell_Cmd : public LCD_UI::UI_Cmd
	{
	public:
		InsertSpell_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour);
		LCD_UI::Collection_Hndl * select(LCD_UI::Collection_Hndl * from) override;
		Collection_Hndl * on_back() override;
		Collection_Hndl * on_select() override;
	private:
		Collection_Hndl & enableInsert(bool enable);
	};

	class Contrast_Brightness_Cmd : public LCD_UI::UI_Cmd
	{
	public:
		enum e_function {e_contrast, e_backlight };
		Contrast_Brightness_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour);
		bool upDown(int moveBy, Collection_Hndl* colln_hndl, LCD_UI::Behaviour ud_behaviour) override;

		LCD_UI::Collection_Hndl * function(e_function fn) { _function = fn; return _self; }

		void setDisplay(HardwareInterfaces::LocalDisplay & lcd);
	private:
		e_function _function = e_contrast;
		HardwareInterfaces::LocalDisplay * _lcd = 0;
		LCD_UI::Collection_Hndl * _self = this;
	};

	class InsertTimeTemp_Cmd : public LCD_UI::UI_Cmd
	{
	public:
		enum TT_Cmds{ e_DelCmd, e_EditCmd, e_NewCmd, e_newLine, e_TTs , e_allCmds = 7, e_none};
		InsertTimeTemp_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour);
		Collection_Hndl * enableCmds(int cmd_to_show);
		bool move_focus_by(int moveBy, Collection_Hndl* colln_hndl) override;
		bool focusHasChanged(bool hasFocus) override ;
		Collection_Hndl * select(Collection_Hndl * from) override;
		bool back() override;
		Collection_Hndl * on_back() override;
		Collection_Hndl * on_select() override;
	private:
		Collection_Hndl _tt_SubPage_h;
		bool _hasInsertedNew = false;
	};

	class TestWatchdog_Cmd : public LCD_UI::UI_Cmd {
	public:
		using LCD_UI::UI_Cmd::UI_Cmd;
		Collection_Hndl* select(Collection_Hndl* from) override { while (true); };
	};
}