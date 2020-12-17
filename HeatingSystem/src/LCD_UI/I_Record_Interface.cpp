#include "I_Record_Interface.h"
#include <RDB.h>

#ifdef ZPSIM
	#include <ostream>
#endif

namespace LCD_UI {
	using namespace RelationalDatabase;
	//****************************************************
	//                Dataset
	//****************************************************

	Dataset::Dataset(I_Record_Interface & recordInterface, Query & query, Dataset * parent) :
		_recSel(query.begin())
		,_parent(parent)
		, _recInterface(&recordInterface)
		,_count(query.end().id())
	{}

	I_Data_Formatter * Dataset::initialiseRecord(int fieldID) {
		query().setMatchArg(parentIndex());
		i_record().answer() = *_recSel.begin();
		setDS_RecordID(_recSel.id());
		return i_record().getField(fieldID);
	}

	int Dataset::resetCount() {
		setMatchArgs();
		query().setMatchArg(parentIndex());
		auto originalID = ds_recordID();
		query().moveTo(_recSel, originalID);
		if (_recSel.id() != originalID) {
			_recSel.begin();
		}
		i_record().answer() = *_recSel;
		setDS_RecordID(_recSel.id());
		_count = query().end().id();
		return _count; 
	}

	int Dataset::parentIndex() const {
		if (_parent == 0) return 0;
		return _parent->ds_recordID();
	}

	I_Data_Formatter * Dataset::getFieldAt(int fieldID, int id) { // moves to first valid record at id or past id from current position
		//if (ds_recordID() != id) {
			setMatchArgs();
			move_to(id);
		//}
		return i_record().getField(fieldID);
	}

	TB_Status Dataset::move_by(int move) {
		query().setMatchArg(parentIndex());
		RecordSelector editRS;
		RecordSelector & recSel = (_inEdit ? editRS = query().resultsQ().begin() : _recSel);
		recSel.setID(ds_recordID());
		i_record().answer() = *(recSel += move);
		setDS_RecordID(recSel.signed_id());
		return recSel.status();
	}

	int Dataset::last() {
		query().setMatchArg(parentIndex());
		RecordSelector editRS;
		RecordSelector & recSel = (_inEdit ? editRS = query().resultsQ().begin() : _recSel);
		recSel.end();
		--recSel;
		i_record().answer() = *recSel;
		return recSel.signed_id();
	}

	int Dataset::move_to(int pos) {
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
		//if (ds_recordID() == pos && recSel.id() == pos)
		//	return pos;

		recSel.setID(ds_recordID());
		if (pos < 0) {
			i_record().answer() = *(recSel.begin());
			i_record().answer().setStatus(TB_BEFORE_BEGIN);
			setDS_RecordID(-1);
		}
		else if (pos == 0) {
			i_record().answer() = *(recSel.begin());
			setDS_RecordID(recSel.signed_id());
		}
		else {
			int direction = pos >= recSel.signed_id() ? 1 : -1;
			auto wasNotAtEndStop = i_record().status() != TB_END_STOP;
			// Refresh current Record in case parent has changed 
			bool noMoveRequested = recSel.signed_id() == pos;
			if (recSel.signed_id() >= 0) {
				recSel.query().next(recSel, 0);
			}
			i_record().answer() = *recSel;

			if (wasNotAtEndStop && i_record().status() == TB_END_STOP) {
				i_record().answer() = *(--recSel);
				//matchID = recSel.signed_id();
			}
			int matchID = recSel.signed_id();
			setDS_RecordID(matchID);

			auto copyAnswer = i_record().answer();
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
				i_record().answer() = copyAnswer;
				setDS_RecordID(matchID);
			}
		}
		return ds_recordID();
	}

	bool Dataset::setNewValue(int fieldID, const I_Data_Formatter* val) {
		i_record().setNewValue(fieldID, val);
		auto newID = i_record().recordID();
		bool hasMoved = newID != ds_recordID();
		setDS_RecordID(newID);
		return hasMoved;
	}

	void Dataset::insertNewData() {
		i_record().answer() = i_record().duplicateRecord(_recSel);
		setDS_RecordID(i_record().recordID());
	}

	void Dataset::deleteData() {
		_recSel.deleteRecord();
		setDS_RecordID(_recSel.id());
	}
}