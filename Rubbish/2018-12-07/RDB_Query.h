#pragma once
#include "RDB_RecordSelector.h"
#include "RDB_TableNavigator.h"
#include <ostream>

namespace RelationalDatabase {

	/// <summary>
	/// A Query is a Rangeable RecordSet lazy generator.
	/// It applies a given rule to one or more source tables or sub - queries
	/// such that the records it returns satisfy the Query criteria.
	/// It provides RecordSelectors from begin(), last() and end() which can iterate over the Query.
	/// RecordSelectors delegate to the Query for Increment and dereferencing.
	/// begin() points to the first valid record.
	/// begin() may be decremented to return an ID of -1, with status TB_BEFORE_BEGIN.
	/// end() has an ID of Table_Capacity and a status of TB_END_STOP.
	/// last() points to the last valid record - also obtained by decrementing end().
	/// It maintains it current position and provides next(moveBy) to support bi-directional Query traversal.
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
		// Range Functions
		virtual RecordSelector begin();
		virtual RecordSelector end();
		virtual RecordSelector last();
		virtual void moveTo(RecordSelector & rs, int index);
		virtual Answer_Locator operator[](int index);
		virtual void next(RecordSelector & recSel, int moveBy);

		// Modify Query
		virtual void setMatchArg(int matchArg) {}
		virtual RecordSelector findMatch(RecordSelector & recSel);
		
		template <typename Record_T>
		RecordSelector insert(const Record_T * record) {
			return incrementTableQ().insert(record);
		}

		//template <typename Record_T>
		//RecordSelector insert(const Record_T * record, int noOfRecords) {
		//	auto rs = RecordSelector{ *this, *_table };
		//	for (int i = 0; i < noOfRecords; ++i) {
		//		incrementTableQ().insert(record);
		//		++record;
		//	}
		//	return rs;
		//}

		// Access
		const Query & incrementQ() const { return const_cast<Query*>(this)->resultsQ(); }
		virtual Query & incrementQ() { return *this; }
		virtual TableQuery & incrementTableQ() { return incrementQ().incrementTableQ(); }
		const Query & resultsQ() const { return const_cast<Query *>(this)->resultsQ(); }
		virtual Query & resultsQ() {return *this;}
		virtual int matchArg() const { return 0; }

		virtual ~Query() = default;
		virtual Answer_Locator getMatch(RecordSelector & recSel, int direction, int id) { return recSel; }
	protected:
		Query() = default;
	};

	class Null_Query : public Query {
	public:
		RecordSelector begin() override { return { *this, 0, TB_INVALID_TABLE }; }
		RecordSelector end() override { return { *this, 1, TB_INVALID_TABLE }; }
		RecordSelector last() override { return begin(); }
		void moveTo(RecordSelector & rs, int index) override {}
	private:
	};
	extern Null_Query nullQuery;

	///////////////////////////////////////////
	//           Simple Table Query          //
	//        Table                          //
	// RecSel->[0]                           //
	//         [1]                           //
	///////////////////////////////////////////

	/// <summary>
	/// A Query on a Table.
	/// Returned from RBD::table(index);
	/// It has a TableNavigator initialised by the constructor.
	/// I maintains its own RecordSelector, recording the Record ID for this query.
	/// Multiple TableQueries may range over the same Table.
	/// TableNavigator maintains the address details for its active iterator.
	/// </summary>
	class TableQuery : public Query {
	public:
		TableQuery() = default;
		TableQuery(Table & table) : _table(&table) {
			std::cout << " TableQuery at: " << std::hex << (long long)this << " TableID : " << std::dec << (int)_table->tableID() << std::endl;
		}

		TableQuery(const TableQuery & tableQ) : _table(tableQ._table) {
			std::cout << " Copy TableQuery at: " << std::hex << (long long)this << " TableID : " << std::dec << (int)_table->tableID() << std::endl;
		}

		RecordSelector begin() override;
		RecordSelector end() override;
		RecordSelector last() override;
		void next(RecordSelector & recSel, int moveBy) override;
		Answer_Locator operator[](int index) override;
		void moveTo(RecordSelector & rs, int index) override;
		TableQuery & incrementTableQ() override { return *this; }

		template <typename Record_T>
		RecordSelector insert(const Record_T * record) {
			auto rs = RecordSelector{ *this, _table };
			rs.tableNavigator().insert(record);
			return rs;
		}

		template <typename Record_T>
		RecordSelector insert(const Record_T * record, int noOfRecords) {
			auto rs = RecordSelector{ *this, _table };
			for (int i = 0; i < noOfRecords; ++i) {
				rs.tableNavigator().insert(record);
				++record;
			}
			return rs;
		}

	private:
		Table * _table = 0;
	};

	class CustomQuery : public Query {
	public:
		CustomQuery(Query & source) : _incrementQ(&source) {}
		CustomQuery(const TableQuery & tableQ) : _incrementQ(&_tableQ), _tableQ(tableQ) {
			std::cout << " Custom Query at : " << std::hex << (long long)this << "\n";
			std::cout << "    Nested Table Query at : " << std::hex << (long long)&_tableQ << "\n";
		}

		// Modify Query	
		template<typename Record_T>
		RecordSelector insert(const Record_T * record) { 
			tableNavigator().insert(record); 
			return current(); 
		}

		// Access
		int matchArg() const override { return _matchArg; }
		Query & incrementQ() override { return *_incrementQ; }
		Query & resultsQ() override { return *_incrementQ; }

		void setMatchArg(int matchArg) override { _matchArg = matchArg; }
	private:
		Query * _incrementQ;
		RecordID _matchArg = 0;
		TableQuery _tableQ;
	};

	///////////////////////////////////////////
	//           Table Filter Query          //
	//_______________________________________//
	//      ResultsTable, match field {0}    //
	//               {0} {1}                 //
	//            [0] 1, fred                //
	// match(0)-> [1] 0, john -> john        //
	///////////////////////////////////////////

	/// <summary>
	/// Single-table filter query.
	/// Returns records whose field-value matches the firstMatch argument.
	/// Provide a TableNavigator,
	/// and the field index for the filter field.
	/// </summary>	
	template <typename ReturnedRecordType>
	class QueryF_T : public CustomQuery {
	public:
		QueryF_T(Query & result_Query, int matchField) : CustomQuery(result_Query), _match_f(matchField) {}
		QueryF_T(const TableQuery & result_Query, int matchField) : CustomQuery(result_Query), _match_f(matchField) {
			std::cout << " Filter Query at : " << std::hex << (long long)this << "\n";
		}

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int id) override {
			auto matchLocator = recSel.incrementRecord();
			Answer_R<ReturnedRecordType> match = matchLocator;
			match.status();
			//std::cout << "  Filter field at ID: " << (int)match.id() << " with val " << (int)match.field(_match_f) << " to " << id << std::endl;
			while (recSel.status() == TB_OK && match.status() == TB_OK && match.field(_match_f) != id) {
				incrementQ().next(recSel, direction);
				match = recSel.incrementRecord();
				//std::cout << "      Try next Match at ID: " << (int)match.id() << " with val " << (int)match.field(_match_f) << std::endl;
			}
			return match;
		}

	private:
		int _match_f;
	};
	
	/////////////////////////////////////////////////////////
	//            Link-Table Query                         //
	//_____________________________________________________//
	// LinkTable, link field {0}{1}      ResultsTable      //
	//               {0} {1}              {0} {1}          //
	// match(1)-> [0] 1,  1---\        [0] 1, fred         //
	//            [1] 0,  0    \------>[1] 0, john -> john //
	/////////////////////////////////////////////////////////

	/// <summary>
	/// Link-Table query.
	/// A Select Query on the Results table, using Link-Table select-field,
	/// where the Link-Table is Filtered on the Match argument.
	/// Provide a Link-Table Query, 
	/// a Result-Table Query,
	/// the field_index in the Link table to filter on the Match argument,
	/// and the field_index in the Link table for selecting the record in the Result-Table.
	/// </summary>	
	template <typename LinkRecordType, typename ReturnedRecordType >
	class QueryL_T : public CustomQuery {
		// base-class holds the match argument for the Filter.
	public:
		using type = typename ReturnedRecordType;

		QueryL_T(
			Query & link_Query,
			Query & result_Query,
			int filter_f,
			int select_f
		) : CustomQuery(result_Query), _linkQ(link_Query, filter_f ), _select_f{ select_f } {
			std::cout << " Link Query and results at : " << std::hex <<(long long)this << "\n";
			std::cout << "    nested Filter Query at : " << std::hex <<(long long)&_linkQ << "\n";
		}

		QueryL_T(
			const TableQuery & link_Query,
			const TableQuery & result_Query,
			int filter_f,
			int select_f
		) : CustomQuery(result_Query), _linkQ(link_Query, filter_f ), _select_f{ select_f } {
			std::cout << " Link Query and results at : " << std::hex << (long long)this << "\n";
			std::cout << "    nested Filter Query at : " << std::hex << (long long)&_linkQ << "\n";
		}
		void setMatchArg(int matchArg) override { _linkQ.setMatchArg(matchArg); }
		int matchArg() const override { return _linkQ.matchArg(); }

	private:
		Query & incrementQ() override { return _linkQ; }

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int) override {
			Answer_R<LinkRecordType> match = recSel.incrementRecord();
			if (match.status() == TB_OK) {
				auto matchField = match.field(_select_f);
				//std::cout << "  Link field at ID: " << (int)match.id() << " to select ID: " << (int)matchField << std::endl;
				return resultsQ()[match.field(_select_f)];
			} else if (match.status() == TB_BEFORE_BEGIN)  {
				return resultsQ()[-1];
			} else return resultsQ().end();
		}

		QueryF_T<LinkRecordType> _linkQ;
		int _select_f; // Field in Link-Table used to select Results
	};
	
	////////////////////////////////////////////////////////////
	//          Lookup Filter Query                           //
	//________________________________________________________//
	// ResultsTable filter {1}  LookupTable, match field {1}  //
	//             {0}  {1}         {0} {1}                   //
	// fred <- [0] fred, 0----->[0] SW1, 1 <-match(1)         //
	// john <- [1] john, 2--\   [1] SW2, 0                    //
	//         [2] mary, 1   \->[2] SW3, 1                    //
	////////////////////////////////////////////////////////////

	/// <summary>
	/// Lookup filter query.
	/// Returns records where the lookup-record field-value matches the firstMatch argument.
	/// Provide a Results-Table Query to be filtered,
	/// a Lookup-Table Query, 
	/// the field_index in the Results-Table for selecting the record in the Lookup-Table.
	/// and the field_index in the Lookup-Table to match with the firstMatch argument.
	/// </summary>	
	template <typename ReturnedRecordType, typename FilterRecordType>
	class QueryLF_T : public CustomQuery {
		// all derived queries inherit from the root, rather from each other, so the virtual return-types can be derived-type.
	public:
		using type = typename ReturnedRecordType;

		QueryLF_T(
			Query & result_query
			, Query & filter_query
			, int select_f
			, int filter_f
		) : CustomQuery(result_query), _filterQuery(filter_query, filter_f), _select_f(select_f) {}

		QueryLF_T(
			const TableQuery & result_query
			, const TableQuery & filter_query
			, int select_f
			, int filter_f
		) : CustomQuery(result_query), _filterQuery(filter_query, filter_f), _select_f(select_f) {}

		void setMatchArg(int matchArg) override { _filterQuery.setMatchArg(matchArg); }
		int matchArg() const override { return _filterQuery.matchArg(); }

	private:

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int) override {
			Answer_R<ReturnedRecordType> tryMatch = recSel.incrementRecord();
			while (tryMatch.status() == TB_OK) {
				auto select = tryMatch.field(_select_f);
				auto filter = _filterQuery[select];
				if (filter.status() == TB_OK) break;
				//recSel.next_IncrementQ(direction);
				incrementQ().next(recSel, direction);
				tryMatch = recSel.incrementRecord();
			}
			recSel.setStatus(tryMatch.status() == TB_RECORD_UNUSED ? (direction >= 0 ? TB_END_STOP : TB_BEFORE_BEGIN) : tryMatch.status());
			return tryMatch;
		}

		QueryF_T<FilterRecordType> _filterQuery;
		int _select_f; // Field in Result-Table used to select Lookup
	};

	////////////////////////////////////////////////////
	//            Inner-Join Query                    //
	//________________________________________________//
	// JoinTable, join field {1}     ResultsTable     //
	//                {0}  {1}          {0} {1}       //
	// match(0)-> [0] fred, 1---\    [0] 1, SW1       //
	//            [1] john, 0    \-->[1] 0, SW2 ->SW2 //
	////////////////////////////////////////////////////

	/// <summary>
	/// Inner-Join query.
	/// Selects records from the Results table, using Join-Table select-field,
	/// where the firstMatch argument is the ID of the Join-Table record.
	/// Provide a Join-Table Query, 
	/// a Result-Table Query,
	/// the field_index of the select-field in the Join table,
	/// </summary>	
	template <typename JoinRecordType>
	class QueryIJ_T : public CustomQuery {
		// all derived queries inherit from the root, rather from each other, so the virtual return-types can be derived-type.
	public:

		QueryIJ_T(
			Query & join_query,
			Query & result_query,
			int select_f
		) : CustomQuery(join_query), _result_query(result_query), _select_f(select_f) {}

		QueryIJ_T(
			const TableQuery & join_query,
			const TableQuery & result_query,
			int select_f
		) : CustomQuery(join_query), _resultsTableQ(result_query), _result_query(&_resultsTableQ), _select_f(select_f) {}
		
		Query & resultsQ() override { return *_result_query; }

	private:
		Answer_Locator getMatch(RecordSelector & recSel, int, int) {
			Answer_R<JoinRecordType> select = recSel.incrementRecord();
			if (select.status() == TB_OK) {
				return (*_result_query)[select.field(_select_f)];
			}
			else return _result_query->end();
		}

		TableQuery _resultsTableQ;
		Query * _result_query;
		int _select_f; // Field in Join-Table used to select Results
	};
	
}