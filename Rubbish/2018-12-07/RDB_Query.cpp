#include "RDB_Query.h"
namespace RelationalDatabase {
	Null_Query nullQuery{};

	RecordSelector Query::begin() {
		auto rs = RecordSelector{ *this, incrementQ().begin() };
		getMatch(rs, 1, matchArg());
		return rs;
	}

	RecordSelector Query::end() {
		return {*this, incrementQ().end() };
	}

	RecordSelector Query::last() {
		resultsQ().last();
		auto match = RecordSelector{ *this, incrementQ().last() };
		if (match.status() == TB_OK) getMatch(match, -1, matchArg());
		return { match };
	}

	void Query::next(RecordSelector & recSel, int moveBy) {
		incrementQ().next(recSel, moveBy);
		moveBy = moveBy ? moveBy : 1;
		getMatch(recSel, moveBy, matchArg());
	}

	RecordSelector Query::findMatch(RecordSelector & recSel) {
		getMatch(recSel, 1, matchArg());
		return recSel;
	}

	void Query::moveTo(RecordSelector & rs, int index) {
		incrementQ().moveTo(rs, index);
		incrementQ().next(rs, 0);
		findMatch(rs);
	}

	Answer_Locator Query::operator[](int index) {
		auto rs = incrementQ().begin();
		moveTo(rs, index);
		auto result = getMatch(rs, 1, matchArg());
		if (rs.id() != index) {
			auto reportStatus = result.status();
			if (reportStatus == TB_OK) reportStatus = TB_RECORD_UNUSED;
			result.setStatus(reportStatus);
		}
		return result;
	}

	/////////////////////////////////////////////////////////
	//                      TableQuery                     //
	/////////////////////////////////////////////////////////

	RecordSelector TableQuery::begin() {
		RecordSelector beginRS{ *this, _table };
		if (beginRS.tableNavigator().moveToFirstRecord()) 
			beginRS.tableNavigator().moveToNextUsedRecord(1);
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

	/////////////////////////////////////////////////////////
	//                    CustomQuery                      //
	/////////////////////////////////////////////////////////


}