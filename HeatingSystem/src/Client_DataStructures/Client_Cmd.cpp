#include "Client_Cmd.h"
#include "..\LCD_UI\UI_FieldData.h"
#include "..\LCD_UI\\I_Record_Interface.h"
#include <iostream>

namespace client_data_structures {
	using namespace LCD_UI;

	InsertSpell_Cmd::InsertSpell_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour)
		: UI_Cmd(label_text, onSelect, behaviour)/*, Collection_Hndl(this)*/ {
		std::cout << "InsertSpell_Cmd at: " << std::hex << (long long)this << std::endl;
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

	InsertTimeTemp_Cmd::InsertTimeTemp_Cmd(const char * label_text, LCD_UI::OnSelectFnctr onSelect, LCD_UI::Behaviour behaviour)
		: UI_Cmd(label_text, onSelect, behaviour)/*, Collection_Hndl(this)*/ {
		std::cout << "InsertTimeTemp_Cmd at: " << std::hex << (long long)this << std::endl;
	}

	/////////////////////////////////////////////////////////////////

	Collection_Hndl * InsertTimeTemp_Cmd::enableCmds(int enable) {
		auto ttSubPageHndl = target();
		auto & ttListHndl = *ttSubPageHndl->get()->collection()->item(e_TTs);
		auto & ttDeleteHndl = *ttSubPageHndl->get()->collection()->item(e_DelCmd);
		if (enable) {
			behaviour().make_visible();
			ttListHndl->behaviour().removeBehaviour(Behaviour::b_NewLine);
			ttListHndl->behaviour().make_viewOne();
			ttDeleteHndl->behaviour().make_visible();
			ttSubPageHndl->move_focus_to(e_NewCmd);
			insertCommandForEdit(ttListHndl);
		}
		else {
			behaviour().make_hidden();
			ttListHndl->behaviour().addBehaviour(Behaviour::b_NewLine);
			ttListHndl->behaviour().make_viewAll();
			ttDeleteHndl->behaviour().make_hidden();
			ttSubPageHndl->move_focus_to(e_TTs);
			removeCommandForEdit(ttListHndl);
		}
		return static_cast<Collection_Hndl*>(&ttListHndl);
	}

	Collection_Hndl * InsertTimeTemp_Cmd::select(Collection_Hndl * from) {
		auto ttSubPageHndl = target();
		if (get()->collection()) {
			auto & _tt_SubPage_c = *ttSubPageHndl->get()->collection();
			ttSubPageHndl->move_focus_to(e_TTs);
			auto tt_UI_FieldData = ttSubPageHndl->activeUI()->get();
			auto tt_field_interface_h = ttSubPageHndl->activeUI()->activeUI();
			tt_field_interface_h->setCursorPos();
			auto  ttData = static_cast<UI_FieldData *>(tt_UI_FieldData->collection());
			ttData->insertNewData();
			tt_field_interface_h->get()->edit(tt_field_interface_h);
			static_cast<Field_Interface_h*>(tt_field_interface_h)->setEditFocus(0);
		}
		else // is delete command
			target()->on_back();
		return 0;
	}

	Collection_Hndl * InsertTimeTemp_Cmd::on_select() { // Save insert
		enableCmds(0);
		return  backUI()->on_select();
	}

	bool InsertTimeTemp_Cmd::back() {
		if (get()->isCollection()) {
			enableCmds(0);
		} else static_cast<InsertTimeTemp_Cmd*>(target())->enableCmds(0);
		return false;
	}

	Collection_Hndl * InsertTimeTemp_Cmd::on_back() { // Cancel insert
		auto ttUI_h = enableCmds(0);
		auto ttData = static_cast<UI_FieldData *>(ttUI_h->get()->collection());
		ttData->deleteData();
		ttData->setFocusIndex(ttData->data()->recordID());
		return backUI()->on_back();
	}
	
	Collection_Hndl * InsertTimeTemp_Cmd::deleteTT(int) {
		on_back();
		return 0;
	}
	
}