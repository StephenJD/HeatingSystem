#include "I_Record_Interface.h"
#include <RDB.h>

#ifdef ZPSIM
	#include <ostream>
#endif

namespace LCD_UI {
	using namespace RelationalDatabase;

	I_Record_Interface::I_Record_Interface(Query & query, VolatileData * runtimeData, I_Record_Interface * parent) :
		_recSel(query.begin())
		,_runtimeData(runtimeData)
		,_parent(parent)
		,_count(query.end().id())
	{}

	I_UI_Wrapper * I_Record_Interface::initialiseRecord(int fieldID) {
		query().setMatchArg(parentIndex());
		record() = *_recSel.begin();
		setRecordID(_recSel.id());
		return getField(fieldID);
	}

	int I_Record_Interface::resetCount() {
		query().setMatchArg(parentIndex());
		auto originalID = _recSel.id();
		query().moveTo(_recSel, _recSel.id());
		if (_recSel.id() != originalID) {
			_recSel.begin();
		}
		record() = *_recSel;
		setRecordID(_recSel.id());
		_count = query().end().id();
		return _count; 
	}

	int I_Record_Interface::parentIndex() const { 
		if (_parent == 0) return 0;
		return _parent->recordID();
	}

	
	I_UI_Wrapper * I_Record_Interface::getFieldAt(int fieldID, int id) { // moves to first valid record at id or past id from current position
		move_to(id);
		return getField(fieldID);
	}

	TB_Status I_Record_Interface::move_by(int move) {
		query().setMatchArg(parentIndex());
		record() = *(_recSel += move);
		setRecordID(_recSel.signed_id());
		return record().status();
	}

	int I_Record_Interface::last() {
		query().setMatchArg(parentIndex());
		_recSel.end();
		--_recSel;
		record() = *_recSel;
		return _recSel.signed_id();
	}

	int I_Record_Interface::move_to(int pos) {
		// pos is the required incrementQ().id
		// Move to the next matching record past the current, depending on the direction of traversal 
		//std::cout << " move_to : " << std::dec <<  pos << " Record().id is :"  << (int)record().id() <<  " IncrementQ id: " << query().incrementQ().signed_id() << std::endl;

		query().setMatchArg(parentIndex());
		if (pos < 0) {
			record() = *_recSel.begin();
			record().setStatus(TB_BEFORE_BEGIN);
			setRecordID(-1);
		}
		else if (pos == 0) {
			record() = *_recSel.begin();
			setRecordID(_recSel.signed_id());
		}
		else {
			auto wasNotAtEndStop = record().status() != TB_END_STOP;
			// Refresh current Record in case parent has changed 
			bool noMoveRequested = _recSel.signed_id() == pos;
			if (_recSel.signed_id() >= 0) {
				query().next(_recSel, 0);
			}
			record() = *_recSel;

			int matchID = _recSel.signed_id();

			if (wasNotAtEndStop && record().status() == TB_END_STOP) {
				record() = *(--_recSel);
			}
			int direction = pos > _recSel.signed_id() ? 1 : -1;
			setRecordID(matchID);

			auto copyAnswer = record();
			bool directionSign = direction < 0;
			int difference = pos - matchID;
			bool differenceSign = difference < 0;
			while ( difference && directionSign == differenceSign) {
				copyAnswer = *(_recSel += direction);
				matchID = _recSel.signed_id();
				difference = pos - matchID;
				differenceSign = difference < 0;
				if (copyAnswer.status() != TB_OK) break;
			}
			if (((difference == 0 || directionSign != differenceSign) && copyAnswer.status() == TB_OK) || copyAnswer.status() != TB_OK)
			{
				record() = copyAnswer;
				setRecordID(matchID);
			}
		}
		return recordID();
	}

	void I_Record_Interface::deleteData() {
		_recSel->deleteRecord();
		_recSel += 0;
		if (_recSel.status() == TB_END_STOP) --_recSel;
		setRecordID(_recSel.signed_id());
	}

	void I_Record_Interface::setFieldValue(int fieldID, int value) {
		getField(fieldID);
	}

}