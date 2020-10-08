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

	I_Data_Formatter * I_Record_Interface::initialiseRecord(int fieldID) {
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

	
	I_Data_Formatter * I_Record_Interface::getFieldAt(int fieldID, int id) { // moves to first valid record at id or past id from current position
		move_to(id);
		return getField(fieldID);
	}

	TB_Status I_Record_Interface::move_by(int move) {
		query().setMatchArg(parentIndex());
		RecordSelector editRS;
		RecordSelector & recSel = (_inEdit ? editRS = query().resultsQ().begin() : _recSel);
		recSel.setID(recordID());
		record() = *(recSel += move);
		setRecordID(recSel.signed_id());
		return record().status();
	}

	int I_Record_Interface::last() {
		query().setMatchArg(parentIndex());
		RecordSelector editRS;
		RecordSelector & recSel = (_inEdit ? editRS = query().resultsQ().begin() : _recSel);
		recSel.end();
		--recSel;
		record() = *recSel;
		return recSel.signed_id();
	}

	int I_Record_Interface::move_to(int pos) {
		// lambdas
		auto okAtOrAfterPos = [](int difference, int directionSign, int differenceSign, TB_Status copyAnswerStatus) {return copyAnswerStatus == TB_OK && (difference == 0 || directionSign != differenceSign); };
		auto isSingleMatchQ = [this](TB_Status copyAnswerStatus, RecordSelector & recSel) {
			return copyAnswerStatus != TB_OK  && recSel.begin().id() == (++recSel).id();
		};
		// pos is the required iterationQ().id
		// Move to the next matching record past the current, depending on the direction of traversal 
		//logger() << F(" move_to : ") << L_dec <<  pos << F(" Record().id is :")  << (int)record().id() << F(" IncrementQ id: ") << query().iterationQ().signed_id() << L_endl;

		query().setMatchArg(parentIndex());
		RecordSelector editRS;
		RecordSelector & recSel = (_inEdit ? editRS = query().resultsQ().begin() : _recSel);
		recSel.setID(recordID());
		if (pos < 0) {
			record() = *(recSel.begin());
			record().setStatus(TB_BEFORE_BEGIN);
			setRecordID(-1);
		}
		else if (pos == 0) {
			record() = *(recSel.begin());
			setRecordID(recSel.signed_id());
		}
		else {
			auto wasNotAtEndStop = record().status() != TB_END_STOP;
			// Refresh current Record in case parent has changed 
			bool noMoveRequested = recSel.signed_id() == pos;
			if (recSel.signed_id() >= 0) {
				recSel.query().next(recSel, 0);
			}
			record() = *recSel;


			if (wasNotAtEndStop && record().status() == TB_END_STOP) {
				record() = *(--recSel);
				//matchID = recSel.signed_id();
			}
			int matchID = recSel.signed_id();
			setRecordID(matchID);

			int direction = pos > recSel.signed_id() ? 1 : -1;
			auto copyAnswer = record();
			auto copyAnswerStatus = copyAnswer.status();
			bool directionSign = direction < 0;
			int difference = pos - matchID;
			bool differenceSign = difference < 0;
			while (difference && directionSign == differenceSign) {
				recSel += direction;
				copyAnswer = recSel;
				copyAnswerStatus = copyAnswer.status();
				matchID = recSel.signed_id();
				difference = pos - matchID;
				differenceSign = difference < 0;
				if (copyAnswerStatus != TB_OK) break;
			}
			if (okAtOrAfterPos(difference, directionSign, differenceSign, copyAnswerStatus) || !isSingleMatchQ(copyAnswerStatus, recSel)) {
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