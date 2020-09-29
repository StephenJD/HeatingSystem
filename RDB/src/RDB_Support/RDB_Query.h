#pragma once
#include "RDB_RecordSelector.h"
#include "RDB_TableNavigator.h"
#include "RDB_Answer_Locator.h"
#include <Logging.h>

#ifdef ZPSIM
	#include <ostream>
#endif

namespace RelationalDatabase {

	/// <summary>
	/// A Query is a Rangeable RecordSet lazy generator.
	/// It applies a given rule to one or more source tables or sub - queries
	/// such that the records it returns satisfy the Query criteria.
	/// It provides begin(), last() and end() returning RecordSelectors which can iterate over the Query.
	/// RecordSelectors delegate to the Query for Increment and dereferencing.
	/// begin() points to the first valid record.
	/// begin() may be decremented to return an ID of -1, with status TB_BEFORE_BEGIN.
	/// end() has an ID of Table_Capacity and a status of TB_END_STOP.
	/// last() points to the last valid record - also obtained by decrementing end().
	/// It provides next(moveBy) to support bi-directional Query traversal on the supplied Record Selector.
	/// Queries may also be indexed, like an array, giving the Record at that ID if it exists, or status of "Record Unused".
	/// A Query may be constructed from another Query and use Match Field arguments to create filters and other more complex Queries by implementing next().
	/// Specialised TableQuery downcasts its Query* member to the TableNavigator passed into its constructor. 
	/// </summary>	

	class TableQuery;

	/// <summary>
	/// An abstract Stateless Query providing iterators.
	/// </summary>

	class Query {
	public:
		// Non-Polymorphic Functions
		RecordSelector begin();
		RecordSelector end();
		RecordSelector last();

		// Polymorphic Increment Functions
		virtual void next(RecordSelector & recSel, int moveBy);
		virtual Answer_Locator getMatch(RecordSelector & recSel, int direction, int matchArg) { return recSel; }

		virtual void moveTo(RecordSelector & rs, int index);
		virtual Answer_Locator operator[](int index);

		// Modify Query
		//virtual void refresh();
		virtual void setMatchArg(int matchArg) {}
		virtual Answer_Locator acceptMatch(int recordID) { return { 0 }; }
		virtual RecordSelector findMatch(RecordSelector & recSel);
		
		template <typename Record_T>
		RecordSelector insert(const Record_T & record);

		// Access
		const Query & iterationQ() const { return const_cast<Query*>(this)->resultsQ(); }
		virtual Query & iterationQ() { return *this; }
		virtual TableQuery & incrementTableQ() { return iterationQ().incrementTableQ(); }
		const Query & resultsQ() const { return const_cast<Query *>(this)->resultsQ(); }
		virtual Query & resultsQ() {return *this;}
		virtual int matchArg() const { return 0; }
		RDB_B * getDB();

		virtual ~Query() = default;
	protected:
		Query() = default;
	};

	///////////////////////////////////////////
	//           Simple Table Query          //
	//        Table                          //
	// RecSel->[0]                           //
	//         [1]                           //
	///////////////////////////////////////////

	/// <summary>
	/// A Query on a Table.
	/// Returned from RBD::table(index);
	/// Multiple TableQueries may range over the same Table.
	/// </summary>
	class TableQuery : public Query {
	public:
		TableQuery() = default;
		TableQuery(Table & table) : _table(&table) { }

		TableQuery(const TableQuery & tableQ) : _table(tableQ._table) {	}

		// Non-Polymorphic specialisations
		RecordSelector begin();
		RecordSelector end();
		RecordSelector last();

		// Polymorphic specialisations
		void next(RecordSelector & recSel, int moveBy) override;
		TableQuery & iterationQ() override { return *this; }

		Answer_Locator operator[](int index) override;
		void moveTo(RecordSelector & rs, int index) override;
		TableQuery & incrementTableQ() override { return *this; }
		Answer_Locator acceptMatch(int recordID) override { return (*this)[recordID]; }

		// New Non-Polymorphic modifiers	
		template <typename Record_T>
		RecordSelector insert(const Record_T & record) {
			static_assert(!typetraits::is_pointer<Record_T>::value, "The argument to insert must not be a pointer.");
			auto rs = RecordSelector{ *this, _table };
			//rs.end();
			rs.tableNavigator().insert(record);
			return rs;
		}

		template <typename Record_T>
		RecordSelector insert(const Record_T * record, int noOfRecords) {
			auto rs = RecordSelector{ *this, _table };
			for (int i = 0; i < noOfRecords; ++i) {
				rs.tableNavigator().insert(*record);
				++record;
			}
			return rs;
		}
		RDB_B * getDB();
		//void refresh() override;
	private:
		Table * _table = 0;
	};

	///////////////////////////////////////////
	//           Match Table Query           //
	//        Table                          //
	// Match(0)->[0]                         //
	//           [1]                         //
	///////////////////////////////////////////

	/// <summary>
	/// A Query on a Table selected by the matchArg.
	/// Returned from RBD::table(matchArg);
	/// </summary>
	class QueryM_T : public TableQuery {
	public:
		using TableQuery::TableQuery;
		QueryM_T(const TableQuery & tableQ) :TableQuery(tableQ) {}
		
		// Non-Polymorphic specialisations
		RecordSelector begin() { return TableQuery::begin(); }
		RecordSelector end() { return TableQuery::end(); }
		RecordSelector last() { return TableQuery::last(); }

		// Polymorphic specialisations
		int matchArg() const override { return _matchArg; }
		void setMatchArg(int matchArg) override { _matchArg = matchArg; }
		Answer_Locator acceptMatch(int recordID) override { _matchArg = recordID; return TableQuery::operator[](recordID); }
		
		Answer_Locator operator[](int index) override {
			return acceptMatch(index);
		}

		void next(RecordSelector & recSel, int moveBy) override {
			if (moveBy == 0) {
				getMatch(recSel, 1, 0);
			} else if (moveBy > 0) {
				recSel = recSel.query().end();
			} else if (moveBy < 0) {
				recSel.setID(-1);
				recSel.setStatus(TB_BEFORE_BEGIN);
			}
		}

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int) override {
			recSel.tableNavigator().moveToThisRecord(_matchArg);
			return TableQuery::getMatch(recSel, direction, 0);
		}

	private:
		RecordID _matchArg = 0;
	};

	template <typename Record_T>
	RecordSelector Query::insert(const Record_T & record) {
		static_assert(!typetraits::is_pointer<Record_T>::value, "The argument to insert must not be a pointer.");
		return incrementTableQ().insert(record);
	}

	class Null_Query : public Query {
	public:
		TableQuery & incrementTableQ() override { return _nulltableQ; }
		void next(RecordSelector & recSel, int moveBy) override { recSel.setID(0); recSel.setStatus(TB_INVALID_TABLE); }

		void moveTo(RecordSelector & rs, int index) override {}
	private:
		TableQuery _nulltableQ = {};
	};
	extern Null_Query nullQuery;

	/// <summary>
	/// A base Query for building complex queries.
	/// It has a TableQuery member to enable construction from temporary TableQuery.
	/// It adds iterationQ() and resultsQ() referring to the sub-queries used for incrementing and returning the matching record.
	/// </summary>	
	class CustomQuery : public Query {
	public:
		CustomQuery(Query & source) : _query(&source) {}
		CustomQuery(const TableQuery & tableQ) : _query(&_tableQ), _tableQ(tableQ) { }

		// Access
		Query & iterationQ() override { return *_query; }
		Query & resultsQ() override { return *_query; }

		Answer_Locator acceptMatch(int recordID) override { return iterationQ().acceptMatch(recordID); }

	private:
		Query * _query;
		TableQuery _tableQ;
	};

	///////////////////////////////////////////
	//           Filter Query                //
	//_______________________________________//
	//      IteratedTable, field {0}         //
	// match(0)-------v                      //
	//            [0] 1, fred                //
	//            [1] 0, john -> john        //
	///////////////////////////////////////////

	/// <summary>
	/// Filter query.
	/// It adds matchArg used to filter the returned records.
	/// Returns records whose field-value matches the Match argument.
	/// Provide a Query,
	/// and the field index for the filter field.
	/// </summary>	
	template <typename IteratedRecordType>
	class QueryF_T : public CustomQuery {
	public:
		QueryF_T(Query & iterationQuery, int matchField) : CustomQuery(iterationQuery), _match_f(matchField) {}
		QueryF_T(const TableQuery & iterationQuery, int matchField) : CustomQuery(iterationQuery), _match_f(matchField) {}

		// Non-Polymorphic specialisations
		RecordSelector begin() {
			auto rs = RecordSelector{ *this, CustomQuery::iterationQ().begin() };
			QueryF_T::getMatch(rs, 1, matchArg());
			return rs;
		}

		RecordSelector end() {
			return { *this, CustomQuery::iterationQ().end() };
		}

		RecordSelector last() {
			auto match = RecordSelector{ *this, CustomQuery::iterationQ().last() };
			if (match.status() == TB_OK) QueryF_T::getMatch(match, -1, matchArg());
			return { match };
		}

		// Polymorphic specialisations
		void setMatchArg(int matchArg) override { _matchArg = matchArg; }
		int matchArg() const override { return _matchArg; }

		Answer_Locator acceptMatch(int recordID) override {
			Answer_R<IteratedRecordType> currRec = iterationQ().acceptMatch(recordID);
			QueryF_T::setMatchArg(currRec.field(_match_f));
			return currRec;
		}		
		
		void next(RecordSelector & recSel, int moveBy) override {
			CustomQuery::iterationQ().next(recSel, moveBy);
			moveBy = moveBy ? moveBy : 1;
			QueryF_T::getMatch(recSel, moveBy, matchArg());
		}


		Answer_Locator getMatch(RecordSelector & recSel, int direction, int matchArg) /*override*/ {
			auto matchLocator = recSel.incrementRecord();
			Answer_R<IteratedRecordType> match = matchLocator;
			match.status();
			while (recSel.status() == TB_OK && match.status() == TB_OK && match.field(_match_f) != matchArg) {
				iterationQ().next(recSel, direction);
				match = recSel.incrementRecord();
			}
			return match;
		}

	private:
		RecordID _matchArg = 0;
		int _match_f;
	};
	
	/////////////////////////////////////////////////////////
	//            Lookup Query                         //
	//_____________________________________________________//
	// IteratedTable, field{1}      ResultsTable           //
	//                                                     //
	//            [0] a, 1---\        [0] fred             //
	//            [1] b, 0    \------>[1] john -> john     //
	//            [2] c, 2------------[2] jim  -> jim      //
	/////////////////////////////////////////////////////////

	/// <summary>
	/// Lookup query.
	/// A Select Query on the Results table, using Iterated-Table select-field,
	/// Provide a Iterated-Table Query, 
	/// a Result-Table Query,
	/// and the field_index in the Iterated table for selecting the record in the Result-Table.
	/// </summary>	
	template <typename IteratedRecordType>
	class QueryL_T : public CustomQuery {
	public:
		QueryL_T(
			Query & iterationQuery,
			Query & result_Query,
			int select_f
		) : CustomQuery(iterationQuery), _resultQ(result_Query), _select_f{ select_f } {}

		QueryL_T(
			const TableQuery & iterationQuery,
			const TableQuery & result_Query,
			int select_f
		) : CustomQuery(iterationQuery), _resultQ(result_Query), _select_f{ select_f } {}
		
		QueryL_T(
			Query & iterationQuery,
			const TableQuery & result_Query,
			int select_f
		) : CustomQuery(iterationQuery), _resultQ(result_Query), _select_f{ select_f } {}
		
		QueryL_T(
			const TableQuery & iterationQuery,
			Query & result_Query,
			int select_f
		) : CustomQuery(iterationQuery), _resultQ(result_Query), _select_f{ select_f } {}
		
		Query & resultsQ() override { return _resultQ.resultsQ(); }

		Answer_Locator acceptMatch(int recordID) override {
			// the resultsQ might be a lookup-filter. It must have its match-arg set.
			Answer_R<IteratedRecordType> currRec = iterationQ().acceptMatch(recordID);
			return _resultQ.acceptMatch(currRec.field(_select_f));  
		}

		Answer_Locator operator[](int index) override {
			// the resultsQ might be a lookup-filter. It must have its match-arg set.
			auto linkRec = RecordSelector{ *this, CustomQuery::operator[](index) };
			return getMatch(linkRec,1,0);
		}

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int) override {
			Answer_R<IteratedRecordType> match = recSel.incrementRecord();
			if (match.status() == TB_OK) {
				auto selectID = match.field(_select_f);
				return resultsQ().CustomQuery::operator[](selectID);
			} else if (match.status() == TB_BEFORE_BEGIN)  {
				return resultsQ().CustomQuery::operator[](-1);
			} else return resultsQ().end();
		}

		int select_f() const { return _select_f; };

	protected:
		CustomQuery _resultQ;
		int _select_f; // Field in IteratedTable used to select Results
	};
	
	/////////////////////////////////////////////////////////
	//            Filtered-Lookup Query                  //
	//_____________________________________________________//
	// IteratedTable, fields {0}{1}   ResultsTable         //
	// match(1)-------v                                    //
	//            [0] 1,  1---\        [0] fred            //
	//            [1] 0,  0    \------>[1] john -> john    //
	//            [2] 1,  2------------[2] jim  -> jim     //
	/////////////////////////////////////////////////////////

	/// <summary>
	/// Filtered-Lookup query.
	/// A Select Query on the Results table, using Iterated-Table select-field,
	/// where the Iterated-Table is Filtered on the Match argument.
	/// Provide an Iterated-Table Query, 
	/// a Result-Table Query,
	/// the field_index in the Iterated table to filter on the Match argument,
	/// and the field_index in the Iterated table for selecting the record in the Result-Table.
	/// </summary>	
	template <typename IteratedRecordType>
	class QueryFL_T : public QueryF_T<IteratedRecordType> {
		// base-class holds the match argument for the Filter.
	public:
		QueryFL_T(
			Query & iterationQuery,
			Query & result_Query,
			int filter_f,
			int select_f
		) : QueryF_T<IteratedRecordType>(iterationQuery, filter_f), _resultQ(iterationQuery, result_Query, select_f) {}

		QueryFL_T(
			const TableQuery & iterationQuery,
			const TableQuery & result_Query,
			int filter_f,
			int select_f
		) : QueryF_T<IteratedRecordType>(iterationQuery, filter_f), _resultQ(iterationQuery, result_Query, select_f) {}

		QueryFL_T(
			Query & iterationQuery,
			const TableQuery & result_Query,
			int filter_f,
			int select_f
		) : QueryF_T<IteratedRecordType>(iterationQuery, filter_f), _resultQ(iterationQuery, result_Query, select_f) {}

		QueryFL_T(
			const TableQuery & iterationQuery,
			Query & result_Query,
			int filter_f,
			int select_f
		) : QueryF_T<IteratedRecordType>(iterationQuery, filter_f), _resultQ(iterationQuery, result_Query, select_f) {}

		Query & resultsQ() override { return _resultQ.resultsQ(); }

		Answer_Locator acceptMatch(int recordID) override {
			// the resultsQ might be a lookup-filter. It must have its match-arg set.
			QueryF_T<IteratedRecordType>::acceptMatch(recordID);
			return _resultQ.acceptMatch(recordID);
		}

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int matchArg) override {
			QueryF_T<IteratedRecordType>::getMatch(recSel, direction, matchArg);
			return _resultQ.getMatch(recSel, direction, matchArg);
		}

	protected:
		QueryL_T<IteratedRecordType> _resultQ;
	};
	
	////////////////////////////////////////////////////////////
	//          Lookup Filter Query                           //
	//________________________________________________________//
	// IteratedTable field{1,1}  FilterTable, match field {1}  //
	//             {0}  {1}         {0}  v----match(1)        //
	// fred <- [0] fred, 0----->[0] SW1, 1                    //
	// john <- [1] john, 2--\   [1] SW2, 0                    //
	//         [2] mary, 1   \->[2] SW3, 1                    //
	////////////////////////////////////////////////////////////

	/// <summary>
	/// Lookup filter query.
	/// Returns records where the associated Filter-record match-field matches the Match argument.
	/// Provide an IteratedTable Query to be filtered,
	/// a FilterTable Query, 
	/// the field_index in the IteratedTable for selecting the record in the FilterTable.
	/// and the field_index in the FilterTable to match with the Match argument.
	/// </summary>	
	template <typename IteratedRecordType, typename FilterRecordType>
	class QueryLF_T : public QueryL_T<IteratedRecordType> {
	public:
		QueryLF_T(
			Query & iterationQuery
			, Query & filter_query
			, int select_f
			, int filter_f
		) : QueryL_T<IteratedRecordType>(iterationQuery, filter_query, select_f), _filterQuery(filter_query, filter_f) {}

		QueryLF_T(
			const TableQuery & iterationQuery
			, const TableQuery & filter_query
			, int select_f
			, int filter_f
		) : QueryL_T<IteratedRecordType>(iterationQuery, filter_query, select_f), _filterQuery(filter_query, filter_f) {}
		
		QueryLF_T(
			Query & iterationQuery
			, const TableQuery & filter_query
			, int select_f
			, int filter_f
		) : QueryL_T<IteratedRecordType>(iterationQuery, filter_query, select_f), _filterQuery(filter_query, filter_f) {}
		
		QueryLF_T(
			const TableQuery & iterationQuery
			, Query & filter_query
			, int select_f
			, int filter_f
		) : QueryL_T<IteratedRecordType>(iterationQuery, filter_query, select_f), _filterQuery(filter_query, filter_f) {}
		
		Query & resultsQ() override { return QueryL_T<IteratedRecordType>::iterationQ(); }

		void setMatchArg(int matchArg) override { _filterQuery.setMatchArg(matchArg); }
		
		Answer_Locator acceptMatch(int recordID) override {
			Answer_R<FilterRecordType> selectedRec = QueryL_T<IteratedRecordType>::acceptMatch(recordID);
			_filterQuery.acceptMatch(selectedRec.id());
			return (*this)[recordID];
		}

		int matchArg() const override { return _filterQuery.matchArg(); }

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int) override {
			Answer_R<IteratedRecordType> tryMatch = recSel.incrementRecord();
			while (tryMatch.status() == TB_OK) {
				auto select = tryMatch.field(_select_f);
				auto filter = _filterQuery[select];
				if (filter.status() == TB_OK) break;
				iterationQ().next(recSel, direction);
				tryMatch = recSel.incrementRecord();
			}
			recSel.setStatus(tryMatch.status() == TB_RECORD_UNUSED ? (direction >= 0 ? TB_END_STOP : TB_BEFORE_BEGIN) : tryMatch.status());
			return tryMatch;
		}

	protected:
		QueryF_T<FilterRecordType> _filterQuery;
	};

	////////////////////////////////////////////////////////////
	//          Linked Filter Query                           //
	//________________________________________________________//
	//    LinkTable{1}       IteratedTable, Filter field {1}  //
	//                {0}  {1}        {0} {1}                 //
	// match(0)-->[0] fred, 0---->[0] SW1, 1 --> SW1          //
	// LF_T[2]--->[1] john, 2-\   [1] SW2, 0                  //
	//            [2] mary, 1  \->[2] SW3, 1 --> SW3          //
	////////////////////////////////////////////////////////////

	/// <summary>
	/// Linked filter query.
	/// Returns IteratedTable records where the Filter-field value matches the Filter-field value of the Linked record.
	/// Provide a LinkTable Query, 
	/// an IteratedTable-Table Query to be filtered,
	/// the field_index in the LinkTable to select the first IteratedTable record.
	/// the field_index in the IteratedTable for filtering.
	/// Link-records are selected using setMatch or indexing[]. This automatically set the matchArg for the filter on the IteratedTable.
	/// Begin/end and next all operate on the IteratedTable for the current filter.
	/// </summary>	
	template <typename LinkRecordType, typename IteratedRecordType>
	class QueryLinkF_T : public QueryF_T<IteratedRecordType> {
	public:
		QueryLinkF_T(
			Query & link_query
			, Query & result_query
			, int select_f
			, int filter_f
		) : QueryF_T<IteratedRecordType>(result_query, filter_f), _linkQuery(link_query, result_query, select_f) {}

		QueryLinkF_T(
			const TableQuery & link_query
			, const TableQuery & result_query
			, int select_f
			, int filter_f
		) : QueryF_T<IteratedRecordType>(result_query, filter_f), _linkQuery(link_query, result_query, select_f) {}

		QueryLinkF_T(
			Query & link_query
			, const TableQuery & result_query
			, int select_f
			, int filter_f
		) : QueryF_T<IteratedRecordType>(result_query, filter_f), _linkQuery(link_query, result_query, select_f) {}

		QueryLinkF_T(
			const TableQuery & link_query
			, Query & result_query
			, int select_f
			, int filter_f
		) : QueryF_T<IteratedRecordType>(result_query, filter_f), _linkQuery(link_query, result_query, select_f) {}

		// Non-polymorphic specialisations
		RecordSelector begin() { return QueryF_T<IteratedRecordType>::begin(); }
		RecordSelector end() { return QueryF_T<IteratedRecordType>::end(); }
		RecordSelector last() { return QueryF_T<IteratedRecordType>::last(); }

		// Polymorphic specialisations
		void next(RecordSelector & recSel, int moveBy) override { QueryF_T<IteratedRecordType>::next(recSel, moveBy); }

		Answer_Locator operator[](int index) override {
			return acceptMatch(index);
		}

		void setMatchArg(int recordID) override {
			Answer_R<IteratedRecordType> currRec = _linkQuery.acceptMatch(recordID);
			QueryF_T<IteratedRecordType>::acceptMatch(currRec.id());
		}

		Answer_Locator acceptMatch(int recordID) override {
			// When accepting match, assume interatedQ has already been aaccepted. 
			return QueryF_T<IteratedRecordType>::acceptMatch(recordID);
		}

	protected:
		QueryL_T<LinkRecordType> _linkQuery;
	};

	////////////////////////////////////////////////////
	//            Matched Lookup Query                    //
	//________________________________________________//
	// IteratedTable, field {1}     ResultsTable      //
	//                     {1}                        //
	// match(0)-->[0] fred, 1---\    [0] 1, SW1       //
	//            [1] john, 0    \-->[1] 0, SW2 ->SW2 //
	////////////////////////////////////////////////////

	/// <summary>
	/// Matched Lookup query is a Lookup Query using matchArg or [] to select the Iterated-record.
	/// Select the unique record from the Results table, using IteratedTable select-field,
	/// where the Match argument is the ID of the Iterated record.
	/// Provide an IteratedTable Query, 
	/// a Result-Table Query,
	/// the field_index of the select-field in the IteratedTable.
	/// Setting the MatchArg or indexing[] sends AcceptMatch message to all nested queries.
	/// </summary>	
	template <typename IteratedRecordType>
	class QueryML_T : public QueryL_T<IteratedRecordType> {
	public:
		QueryML_T(
			Query & iterationQuery,
			Query & result_query,
			int select_f
		) : QueryL_T<IteratedRecordType>{ iterationQuery, result_query, select_f } {}

		QueryML_T(
			const TableQuery & iterationQuery,
			const TableQuery & result_query,
			int select_f
		) : QueryL_T<IteratedRecordType>{ iterationQuery, result_query, select_f } {}
		
		QueryML_T(
			Query & iterationQuery,
			const TableQuery & result_query,
			int select_f
		) : QueryL_T<IteratedRecordType>{ iterationQuery, result_query, select_f } {}
		
		QueryML_T(
			const TableQuery & iterationQuery,
			Query & result_query,
			int select_f
		) : QueryL_T<IteratedRecordType>{ iterationQuery, result_query, select_f } {}
		
		// Non-Polymorphic specialisations
		RecordSelector begin() { return QueryL_T<IteratedRecordType>::begin(); }
		RecordSelector end() { return QueryL_T<IteratedRecordType>::end(); }
		RecordSelector last() { return QueryL_T<IteratedRecordType>::last(); }
		
		// Polymorphic specialisations
		Answer_Locator operator[](int index) override {
			return acceptMatch(index);
		}

		void setMatchArg(int index) override {
			acceptMatch(index);
		}

		int matchArg() const override { return _matchArg; }

		Answer_Locator acceptMatch(int recordID) override {
			_matchArg = recordID;
			return QueryL_T<IteratedRecordType>::acceptMatch(recordID);
		}

		void next(RecordSelector & recSel, int moveBy) override {
			if (moveBy == 0) {
				getMatch(recSel, 1, 0);
			} else if (moveBy > 0) {
				recSel = recSel.query().end();
			} else if (moveBy < 0) {
				recSel.setID(-1);
				recSel.setStatus(TB_BEFORE_BEGIN);
			}
		}
		
		Answer_Locator getMatch(RecordSelector & recSel, int direction, int) override {
			recSel.tableNavigator().moveToThisRecord(_matchArg);
			return QueryL_T<IteratedRecordType>::getMatch(recSel,direction,0);
		}

	protected:
		RecordID _matchArg = 0;

	};
	
}