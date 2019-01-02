#pragma once
#include "RDB_TableNavigator.h"
#include "RDB_AnswerID.h"
#include "RDB_Capacities.h"
#include "..\..\..\Sum_Operators\Sum_Operators.h"

namespace RelationalDatabase {
	using namespace Rel_Ops;
	class AnswerID;
	class Table;
	class Query;
	class Answer_Locator;

	/// <summary>
	/// Iterates over a Query, returning the record when dereferenced.
	/// Queries may be composed from other Queries such that one Query is responsible for iteration
	/// with another responsible for returning the record when dereferenced.
	/// The RecordSelector.id() returns the Increment Query ID.
	/// The Results Record ID must be obtained by querying the dereferenced RecordSelector.
	/// The RecordSelector Iterates over the Increment Query which it always points to.
	/// Advancing the RecordSelector moves it to the next record satisfying the full Composite Query conditions.
	/// Subscripting the RecordSelector returns the Results record at the requested Increment Query ID,
	/// and if the Results record does not satisfying the full Composite Query conditions its status is TB_RECORD_UNUSED.
	/// Each RecordSelector contains its own Increment-TableNavigator for efficient record access.
	/// Knowledge of how to move is in the Query, not the RecordSelector.
	/// </summary>	
	class RecordSelector {
	public:
		RecordSelector() = default;
		RecordSelector(Query & query);
		RecordSelector(Query & query, RecordSelector rs);
		RecordSelector(Query & query, Table * table);
		RecordSelector(Query & query, int id, TB_Status status); // for NullQuery
		RecordSelector & operator=(TableNavigator & tableNavigator);
		
		// Iterator Functions
		bool operator==(const RecordSelector & that) const { return id() == that.id(); }
		bool operator<(const RecordSelector & that) const { return id() < that.id(); }
		bool operator>(const RecordSelector & that) const { return id() > that.id(); }
		
		RecordSelector & begin();
		RecordSelector & end();
		RecordSelector & operator++() { return (*this)+=(1); }
		RecordSelector & operator--() { return (*this) += (-1); }
		RecordSelector & operator+=(int moveBy);
		RecordSelector & operator-=(int moveBy) { return (*this) += (-moveBy); }

		Answer_Locator operator*() const;
		Answer_Locator operator->() const; // must return from the query responsible for the results

		Answer_Locator operator[](int index);
		void next_IncrementQ(int moveBy); // Moves the Increment TableQuery to the next used record, without applying a match.

		// Retrieval
		Answer_Locator incrementRecord() const; // Returns the incrementRecord() insted of the results record.
		TB_Status status() const { return currentPos().status(); }
		int id() const { return currentPos().id(); }
		int signed_id() const { return currentPos().signed_id(); }
		AnswerID currentPos() const { return _tableNav.answerID(); }

		// Modify TableNavigator
		void setStatus(TB_Status status) { _tableNav.setStatus(status); }
		void setID(int id) { _tableNav.setID(id); }
		void nextID(int moveBy);
		void setRecord(AnswerID rec) { _tableNav.setCurrent(rec); }
		TableNavigator & tableNavigator() { return _tableNav; }
		const TableNavigator & tableNavigator() const { return _tableNav; }
		// Query Access
		const Query & query() const { return *_query; }
		Query & query() { return *_query; }

	private:
		Query * _query = 0;
		TableNavigator _tableNav;
	};

}