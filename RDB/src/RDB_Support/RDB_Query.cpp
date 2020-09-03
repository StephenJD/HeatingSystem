#include "RDB_Query.h"
#include "RDB_Table.h"


namespace RelationalDatabase {
	Null_Query nullQuery{};

	//void Query::refresh() { incrementTableQ().refresh(); }

	RecordSelector Query::begin() {
		auto rs = RecordSelector{};
		if (&resultsQ() == this) 
			rs = RecordSelector{ *this, incrementTableQ().begin() };
		else 
			rs = RecordSelector{ *this, iterationQ().begin() };
		getMatch(rs, 1, matchArg());
		return rs;
	}

	RecordSelector Query::end() {
		//if (&resultsQ() == this) 
		//	return {*this, incrementTableQ().end() };
		//else 
			return {*this, iterationQ().incrementTableQ().end() };
	}

	RecordSelector Query::last() {
		if (&resultsQ() != this) resultsQ().last();
		//auto match = RecordSelector{ *this, iterationQ().last() };
		auto match = RecordSelector{ *this, incrementTableQ().last() };
		if (match.status() == TB_OK) getMatch(match, -1, matchArg());
		return { match };
	}

	void Query::next(RecordSelector & recSel, int moveBy) {
		/*if (&iterationQ() != this) */iterationQ().next(recSel, moveBy);
		moveBy = moveBy ? moveBy : 1;
		getMatch(recSel, moveBy, matchArg());
	}

	RecordSelector Query::findMatch(RecordSelector & recSel) {
		getMatch(recSel, 1, matchArg());
		return recSel;
	}

	void Query::moveTo(RecordSelector & rs, int index) {
		if (&iterationQ() != this) 
			iterationQ().moveTo(rs, index);
		iterationQ().next(rs, 0);
		findMatch(rs);
	}

	Answer_Locator Query::operator[](int index) {
		auto rs = iterationQ().end();
		moveTo(rs, index);
		auto result = getMatch(rs, 1, matchArg());
		if (rs.id() != index) {
			auto reportStatus = result.status();
			if (reportStatus == TB_OK) reportStatus = TB_RECORD_UNUSED;
			result.setStatus(reportStatus);
		}
		return result;
	}

	RDB_B * Query::getDB() { return incrementTableQ().getDB(); }

	//Answer_Locator  Query::getMatch(RecordSelector & recSel, int direction, int id) { return recSel; }

	/////////////////////////////////////////////////////////
	//                      TableQuery                     //
	/////////////////////////////////////////////////////////

	//void TableQuery::refresh() {
	//	if (_table != 0) {
	//		_table->refresh();
	//	}
	//}


	RecordSelector TableQuery::begin() {
		RecordSelector beginRS{ *this, _table };
//logger() << 6 << L_endl;

		if (beginRS.tableNavigator().moveToFirstRecord()) {
		//	logger() << 7 << L_endl;
			beginRS.tableNavigator().moveToNextUsedRecord(1);
		//	logger() << 8 << L_endl;
		}
		return beginRS;
	}

	RecordSelector TableQuery::end() {
		RecordSelector endRS{ *this, _table };
		NoOf_Recs_t id = 1;
		if (endRS.status() != TB_INVALID_TABLE) {
			id = endRS.tableNavigator().table().maxRecordsInTable();
		}
		endRS.setRecord({ id, TB_END_STOP });
		return endRS;
	}

	RecordSelector TableQuery::last() {
		RecordSelector lastRS{ *this, _table };
		lastRS.tableNavigator().moveToLastRecord();
		return lastRS;
	}

	void TableQuery::next(RecordSelector & recSel, int moveBy) {
		recSel.nextID(moveBy);
		if (!moveBy) moveBy = 1;
		recSel.tableNavigator().moveToNextUsedRecord(moveBy);
	}

	void TableQuery::moveTo(RecordSelector & recSel, int index) {
		recSel.tableNavigator().moveToThisRecord(index);
	}
	
	Answer_Locator TableQuery::operator[](int index) {
		RecordSelector atRS{ *this, _table };
		moveTo(atRS, index);
		return *atRS;
	}

	RDB_B * TableQuery::getDB() { return _table ? &_table->db() : 0; }


	/////////////////////////////////////////////////////////
	//                    CustomQuery                      //
	/////////////////////////////////////////////////////////


}