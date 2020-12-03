#include "RDB_RecordSelector.h"
#include "RDB_Query.h"

namespace RelationalDatabase {
 
	// *********** RecordSelector ***************
	RecordSelector::RecordSelector(Query & query) : _query(&query) { }
	RecordSelector::RecordSelector(Query & query, RecordSelector rs) : _query(&query), _tableNav(rs.tableNavigator()){}
	RecordSelector::RecordSelector(Query & query, Table * table) : _query(&query), _tableNav(table) { }
	RecordSelector::RecordSelector(Query & query, int id, TB_Status status) : _query(&query), _tableNav(id, status) {}
	
	RecordSelector::RecordSelector(Query & query, const Answer_Locator & answer) : 
		_query(&query), _tableNav(answer) { 
		setID(answer.id()); 
		setStatus(answer.status());
	}

	RecordSelector & RecordSelector::begin() { query().moveTo(*this, 0); return *this; }
	RecordSelector & RecordSelector::end() { query().moveTo(*this, query().end().id()); return *this; }

	RecordSelector & RecordSelector::operator=(TableNavigator & tableNavigator) {
		setRecord(tableNavigator.answerID());
		return *this;
	}

	RecordSelector & RecordSelector::operator+=(int offset) { 
		// must call next on the top-level Query
		int direction = (offset < 0 ? -1 : 1);
		_query->next(*this, direction);
		return *this;
	}

	void RecordSelector::next_IncrementQ(int moveBy) {
		setID(signed_id() + moveBy);
		_query->iterationQ().moveTo(*this, id());
	}

	Answer_Locator RecordSelector::operator*() const {
		RecordSelector & thisRS = const_cast<RecordSelector&>(*this);
		return thisRS.query().getMatch(thisRS,1,query().matchArg());
	}

	Answer_Locator RecordSelector::incrementRecord() const {
		RecordSelector & thisRS = const_cast<RecordSelector&>(*this);
		return thisRS;
	}

	Answer_Locator RecordSelector::operator->() const {
		return operator*();
	}

	void RecordSelector::nextID(int moveBy) {
		setID(signed_id() + moveBy);
	}

	Answer_Locator RecordSelector::operator[](int index) { query().moveTo(*this, index); return *this; }

	void RecordSelector::deleteRecord() {
		operator*().deleteRecord();
		operator++();
		if (status() == TB_END_STOP) 
			operator--();
	}

}