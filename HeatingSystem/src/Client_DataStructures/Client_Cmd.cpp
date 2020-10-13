#include "Client_Cmd.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\LCD_UI\\I_Record_Interface.h"
#include "..\HardwareInterfaces\LocalDisplay.h"

#ifdef ZPSIM
	#include <iostream>
#endif

namespace client_data_structures {
	using namespace LCD_UI;

	///////////////// InsertSpell_Cmd /////////////////////////

	InsertSpell_Cmd::InsertSpell_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour)
		: UI_Cmd(label_text, onSelect, behaviour) {
#ifdef ZPSIM
		logger() << F("InsertSpell_Cmd at: ") << L_hex << (long)this << L_endl;
#endif
	}

	Collection_Hndl &  InsertSpell_Cmd::enableInsert(bool enable) {
		enum { _dwellingCalendarCmd, _insert, _fromCmd, dwellSpellUI_c, spellProgUI_c};
		auto & subPage = *target()->get()->collection();
		auto & spellUIcoll = *subPage.item(dwellSpellUI_c);
		if (enable) {
			subPage.item(_dwellingCalendarCmd)->get()->behaviour().make_hidden();
			subPage.item(_insert)->get()->behaviour().make_visible();
			subPage.move_focus_to(dwellSpellUI_c);
			insertCommandForEdit(spellUIcoll);
		}
		else {
			// Hide commands
			subPage.item(_insert)->get()->behaviour().make_hidden();
			subPage.item(_dwellingCalendarCmd)->get()->behaviour().make_visible();
			subPage.move_focus_to(_fromCmd);
			removeCommandForEdit(spellUIcoll);
		}
		return static_cast<Collection_Hndl&>(*subPage.item(dwellSpellUI_c));
	}

	Collection_Hndl * InsertSpell_Cmd::select(Collection_Hndl * from) {
		auto & dwellSpellUI_h = enableInsert(true);
		// insert new spell
		auto & spellData = static_cast<UI_FieldData &>(*dwellSpellUI_h.get());
		spellData.insertNewData();
		auto dwellSpell_interface_h = dwellSpellUI_h.activeUI();
		dwellSpell_interface_h->setCursorPos();
		// Put dwellSpell in edit.
		dwellSpell_interface_h->get()->select(dwellSpell_interface_h);
		return 0;
	}

	Collection_Hndl * InsertSpell_Cmd::on_select() { // Save insert
		enableInsert(false);
		return  backUI()->on_select();
	}

	Collection_Hndl * InsertSpell_Cmd::on_back() { // Cancel insert
		auto & dwellSpellUI_h = enableInsert(false);
		auto & spellData = static_cast<UI_FieldData &>(*dwellSpellUI_h.get());
		spellData.deleteData();
		return backUI()->on_back(); 
	}

	///////////////// Contrast_Brightness_Cmd /////////////////////////

	Contrast_Brightness_Cmd::Contrast_Brightness_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour)
		: UI_Cmd(label_text, onSelect, behaviour) {
#ifdef ZPSIM
		logger() << F("Contrast_Brightness_Cmd at: ") << L_hex << (long)this << L_endl;
#endif
	}

	bool Contrast_Brightness_Cmd::move_focus_by(int moveBy) {
		if (_function == e_backlight) _lcd->changeBacklight(moveBy);
		else _lcd->changeContrast(moveBy);
		return true;
	}

	void Contrast_Brightness_Cmd::setDisplay(HardwareInterfaces::LocalDisplay & lcd) {
		_lcd = &lcd;
		_lcd->setBackLight(true);
	}

	///////////////// InsertTimeTemp_Cmd /////////////////////////

	InsertTimeTemp_Cmd::InsertTimeTemp_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour)
		: UI_Cmd(label_text, onSelect, behaviour) {
#ifdef ZPSIM
		logger() << F("InsertTimeTemp_Cmd at: ") << L_hex << (long)this << L_endl;
#endif
	}	
	
	bool InsertTimeTemp_Cmd::focusHasChanged(bool moveRight) {
		if (moveRight) {
			enableCmds(e_NewCmd);
		}
		else {
			enableCmds(e_DelCmd);
		}
		return true;
	}

	Collection_Hndl * InsertTimeTemp_Cmd::enableCmds(int cmd_to_show) {
		auto ttSubPageHndl = target();
		auto & ttListHndl = *static_cast<Collection_Hndl *>(ttSubPageHndl->get()->collection()->item(e_TTs));
		auto & ttDeleteHndl = *ttSubPageHndl->get()->collection()->item(e_DelCmd);
		auto & ttNewHndl = *ttSubPageHndl->get()->collection()->item(e_NewCmd);
		auto newFocus = cmd_to_show == e_allCmds ? e_EditCmd : e_TTs;
		if (cmd_to_show == e_DelCmd) {
			newFocus = e_DelCmd;
			_hasInsertedNew = true;
			//ttListHndl->behaviour().removeBehaviour(Behaviour::b_Selectable);
			//ttDeleteHndl->behaviour().make_viewAll();
		} else if (cmd_to_show != e_none) _hasInsertedNew = false;
		behaviour().make_visible(cmd_to_show & e_EditCmd);
		behaviour().make_newLine(cmd_to_show == e_EditCmd);
		ttDeleteHndl->behaviour().make_visible((cmd_to_show == e_allCmds) | (cmd_to_show == e_DelCmd));
		ttNewHndl->behaviour().make_visible(cmd_to_show & e_NewCmd);
		ttNewHndl->behaviour().make_newLine(cmd_to_show == e_NewCmd);
		ttSubPageHndl->move_focus_to(newFocus);

		if (cmd_to_show == e_none) {
			ttListHndl->behaviour().make_viewAll();
			ttListHndl->focusHasChanged(true);
			behaviour().make_viewAll();
			removeCommandForEdit(*ttListHndl.activeUI());
		} else {
			if (cmd_to_show != e_allCmds) behaviour().make_viewOne();
			ttListHndl->behaviour().make_viewOne();
			insertCommandForEdit(*ttListHndl.activeUI());
			if (cmd_to_show == e_NewCmd) select(this);
		}
		return static_cast<Collection_Hndl*>(&ttListHndl);
	}

	Collection_Hndl * InsertTimeTemp_Cmd::select(Collection_Hndl * from) { // start insert new
		auto ttSubPageHndl = target();
		if (get()->collection()) {
			ttSubPageHndl->move_focus_to(e_TTs);
			auto tt_UI_FieldData_h = ttSubPageHndl->activeUI()->activeUI();
			auto tt_field_interface_h = tt_UI_FieldData_h->activeUI();
			tt_field_interface_h->setCursorPos();
			if (behaviour().is_viewAll_LR()) { // must be "Edit" command 
				enableCmds(e_EditCmd);
			}
			else {// must be "New" command 
				auto  ttData = static_cast<UI_FieldData *>(tt_UI_FieldData_h->get()->collection());
				ttData->insertNewData();
				_hasInsertedNew = true;
			}
			tt_field_interface_h->get()->edit(tt_field_interface_h);
			static_cast<Field_StreamingTool_h*>(tt_field_interface_h)->setEditFocus(0);
		}
		else {// is delete command
			target()->on_back();
		}
		return 0;
	}

	Collection_Hndl * InsertTimeTemp_Cmd::on_select() { // Save insert
		enableCmds(e_none);
		return  backUI()->on_select();
	}

	bool InsertTimeTemp_Cmd::back() {
		if (get()->isCollection()) {
			enableCmds(e_none);
		} else static_cast<InsertTimeTemp_Cmd*>(target())->enableCmds(e_none);
		return false;
	}

	Collection_Hndl * InsertTimeTemp_Cmd::on_back() { // Cancel insert/edit
		auto ttUI_h = enableCmds(e_none);
		auto ttData = static_cast<UI_FieldData *>(ttUI_h->activeUI()->get()->collection());
		if (_hasInsertedNew) {
			auto thisTT = ttData->data()->recordID();
			bool isOnlyTT = ttData->data()->last() == ttData->data()->query().begin().id();
			if (!isOnlyTT) {
				ttData->data()->move_to(thisTT);
				ttData->deleteData();
			}
		}
		ttData->setFocusIndex(ttData->data()->recordID());
		return backUI()->on_back();
	}
}