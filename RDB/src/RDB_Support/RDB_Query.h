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
		// Range Functions
		virtual RecordSelector begin();
		virtual RecordSelector end();
		virtual RecordSelector last();
		virtual void moveTo(RecordSelector & rs, int index);
		virtual Answer_Locator operator[](int index);
		virtual void next(RecordSelector & recSel, int moveBy);

		// Modify Query
		//virtual void refresh();
		virtual void setMatchArg(int matchArg) {}
		virtual void acceptMatch(RecordSelector & recSel) {}
		virtual RecordSelector findMatch(RecordSelector & recSel);
		virtual bool uniqueMatch() { return false; }
		
		template <typename Record_T>
		RecordSelector insert(const Record_T & record);

		// Access
		const Query & incrementQ() const { return const_cast<Query*>(this)->resultsQ(); }
		virtual Query & incrementQ() { return *this; }
		virtual TableQuery & incrementTableQ() { return incrementQ().incrementTableQ(); }
		const Query & resultsQ() const { return const_cast<Query *>(this)->resultsQ(); }
		virtual Query & resultsQ() {return *this;}
		virtual int matchArg() const { return 0; }
		RDB_B * getDB();

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
	/// Multiple TableQueries may range over the same Table.
	/// </summary>
	class TableQuery : public Query {
	public:
		TableQuery() = default;
		TableQuery(Table & table) : _table(&table) {

#ifdef ZPSIM
			//logger() << F(" TableQuery at: ") << L_hex << (long)this << F(" TableID : ") << L_dec << (int)_table->tableID() << L_endl;
#endif
		}

		TableQuery(const TableQuery & tableQ) : _table(tableQ._table) {
#ifdef ZPSIM
			//logger() << F(" Copy TableQuery at: ") << L_hex << (long)this << F(" TableID : ") << L_dec << (int)_table->tableID() << L_endl;
#endif
		}

		RecordSelector begin() override;
		RecordSelector end() override;
		RecordSelector last() override;
		void next(RecordSelector & recSel, int moveBy) override;
		Answer_Locator operator[](int index) override;
		void moveTo(RecordSelector & rs, int index) override;
		TableQuery & incrementTableQ() override { return *this; }

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

	template <typename Record_T>
	RecordSelector Query::insert(const Record_T & record) {
		static_assert(!typetraits::is_pointer<Record_T>::value, "The argument to insert must not be a pointer.");
		return incrementTableQ().insert(record);
	}

	/// <summary>
	/// A base Query for building complex queries.
	/// It adds matchArg used to restrict the returned records in some way.
	/// It adds incrementQ() and resultsQ() referring to the sub-queries used for incrementing and returning the matching record.
	/// </summary>	
	class CustomQuery : public Query {
	public:
		CustomQuery(Query & source) : _incrementQ(&source) {}
		CustomQuery(const TableQuery & tableQ) : _incrementQ(&_tableQ), _tableQ(tableQ) {
#ifdef ZPSIM
			//logger() << F(" Custom Query at : ") << L_hex << (long)this << F("\n");
			//logger() << F("    Nested Table Query at : ") << L_hex << (long)&_tableQ << F("\n");
#endif
		}

		// Access
		int matchArg() const override { return _matchArg; }

		void acceptMatch(RecordSelector & recSel) override {
			setMatchArg(recSel.id());
		}

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
	// match(0)-------v                      //
	//            [0] 1, fred                //
	//            [1] 0, john -> john        //
	///////////////////////////////////////////

	/// <summary>
	/// Single-table filter query.
	/// Returns records whose field-value matches the firstMatch argument.
	/// Provide a Query,
	/// and the field index for the filter field.
	/// </summary>	
	template <typename ReturnedRecordType>
	class QueryF_T : public CustomQuery {
	public:
		QueryF_T(Query & result_Query, int matchField) : CustomQuery(result_Query), _match_f(matchField) {}
		QueryF_T(const TableQuery & result_Query, int matchField) : CustomQuery(result_Query), _match_f(matchField) {
#ifdef ZPSIM
			//logger() << F(" Filter Query at : ") << L_hex << (long)this << F("\n");
#endif
		}

		void acceptMatch(RecordSelector & recSel) override {
			Answer_R<ReturnedRecordType> currRec = recSel.incrementRecord();
			CustomQuery::setMatchArg(currRec.field(_match_f));
		}

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int id) override {
			auto matchLocator = recSel.incrementRecord();
			Answer_R<ReturnedRecordType> match = matchLocator;
			match.status();
			//logger() << F("  Filter field at ID: ") << (int)match.id() << F(" with val ") << (int)match.field(_match_f) << F(" to ") << id << L_endl;
			while (recSel.status() == TB_OK && match.status() == TB_OK && match.field(_match_f) != id) {
				incrementQ().next(recSel, direction);
				match = recSel.incrementRecord();
				//match.rec();
				//logger() << F("      Try next Match at ID: ") << (int)match.id() << F(" with val ") << (int)match.field(_match_f) << L_:endl;
			}
			return match;
		}

	private:
		int _match_f;
	};
	
	/////////////////////////////////////////////////////////
	//            Link-Table Query                         //
	//_____________________________________________________//
	// LinkTable,  link field{1}      ResultsTable         //
	//                                                     //
	//            [0] a, 1---\        [0] fred             //
	//            [1] b, 0    \------>[1] john -> john     //
	//            [2] c, 2------------[2] jim  -> jim      //
	/////////////////////////////////////////////////////////

	/// <summary>
	/// Link-Table query.
	/// A Select Query on the Results table, using Link-Table select-field,
	/// Provide a Link-Table Query, 
	/// a Result-Table Query,
	/// and the field_index in the Link table for selecting the record in the Result-Table.
	/// </summary>	
	template <typename LinkRecordType, typename ReturnedRecordType >
	class QueryL_T : public CustomQuery {
	public:
		QueryL_T(
			Query & link_Query,
			Query & result_Query,
			int select_f
		) : CustomQuery(link_Query), _resultQ(result_Query), _select_f{ select_f } {}

		QueryL_T(
			const TableQuery & link_Query,
			const TableQuery & result_Query,
			int select_f
		) : CustomQuery(link_Query), _resultQ(result_Query), _select_f{ select_f } {}
		
	private:

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int) override {
			Answer_R<LinkRecordType> match = recSel.incrementRecord();
			if (match.status() == TB_OK) {
				auto selectID = match.field(_select_f);
				return _resultQ[selectID];
			} else if (match.status() == TB_BEFORE_BEGIN)  {
				return _resultQ[-1];
			} else return _resultQ.end();
		}

		CustomQuery _resultQ;
		int _select_f; // Field in Link-Table used to select Results
	};
	
	/////////////////////////////////////////////////////////
	//            Filter-Link-Table Query                         //
	//_____________________________________________________//
	// LinkTable,  link field {0}{1}      ResultsTable     //
	// match(1)-------v                                    //
	//            [0] 1,  1---\        [0] fred            //
	//            [1] 0,  0    \------>[1] john -> john    //
	//            [2] 1,  2------------[2] jim  -> jim     //
	/////////////////////////////////////////////////////////

	/// <summary>
	/// Filter-Link-Table query.
	/// A Select Query on the Results table, using Link-Table select-field,
	/// where the Link-Table is Filtered on the Match argument.
	/// Provide a Link-Table Query, 
	/// a Result-Table Query,
	/// the field_index in the Link table to filter on the Match argument,
	/// and the field_index in the Link table for selecting the record in the Result-Table.
	/// </summary>	
	template <typename LinkRecordType, typename ReturnedRecordType >
	class QueryFL_T : public CustomQuery {
		// base-class holds the match argument for the Filter.
	public:
		QueryFL_T(
			Query & link_Query,
			Query & result_Query,
			int filter_f,
			int select_f
		) : CustomQuery(result_Query), _linkQ(link_Query, filter_f ), _select_f{ select_f } {
#ifdef ZPSIM
			//logger() << F(" Link Query and results at : ") << L_hex <<(long)this << F("\n");
			//logger() << F("    nested Filter Query at : ") << L_hex <<(long)&_linkQ << F("\n");
#endif
		}

		QueryFL_T(
			const TableQuery & link_Query,
			const TableQuery & result_Query,
			int filter_f,
			int select_f
		) : CustomQuery(result_Query), _linkQ(link_Query, filter_f ), _select_f{ select_f } {
#ifdef ZPSIM
			//logger() << F(" Link Query and results at : ") << L_hex << (long)this << F("\n");
			//logger() << F("    nested Filter Query at : ") << L_hex << (long)&_linkQ << F("\n");
#endif
		}
		void setMatchArg(int matchArg) override { _linkQ.setMatchArg(matchArg); }
		
		void acceptMatch(RecordSelector & recSel) override {
			_linkQ.acceptMatch(recSel);
		}

		int matchArg() const override { return _linkQ.matchArg(); }

	private:
		Query & incrementQ() override { return _linkQ; }

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int) override {
			Answer_R<LinkRecordType> match = recSel.incrementRecord();
			if (match.status() == TB_OK) {
				auto matchField = match.field(_select_f);
				//logger() << F("  Link field at ID: ") << (int)match.id() << F(" to select ID: ") << (int)matchField << L_endl;
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
	//             {0}  {1}         {0}  v----match(1)        //
	// fred <- [0] fred, 0----->[0] SW1, 1                    //
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
	public:
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
		
		void acceptMatch(RecordSelector & recSel) override {
			_filterQuery.acceptMatch(recSel);
		}

		int matchArg() const override { return _filterQuery.matchArg(); }

	private:

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int) override {
			Answer_R<ReturnedRecordType> tryMatch = recSel.incrementRecord();
			while (tryMatch.status() == TB_OK) {
				auto select = tryMatch.field(_select_f);
				auto filter = _filterQuery[select];
				if (filter.status() == TB_OK) break;
				incrementQ().next(recSel, direction);
				tryMatch = recSel.incrementRecord();
			}
			recSel.setStatus(tryMatch.status() == TB_RECORD_UNUSED ? (direction >= 0 ? TB_END_STOP : TB_BEFORE_BEGIN) : tryMatch.status());
			return tryMatch;
		}

		QueryF_T<FilterRecordType> _filterQuery;
		int _select_f; // Field in Result-Table used to select Lookup
	};

	////////////////////////////////////////////////////////////
	//          Link Filter Query                             //
	//________________________________________________________//
	//    LinkTable{1}       ResultTable, Filter field {1}    //
	//             {0}  {1}         {0} {1}                   //
	// match-->[0] fred, 0----->[0] SW1, 1 --> SW1            //
	//         [1] john, 2      [1] SW2, 0                    //
	//         [2] mary, 1      [2] SW3, 1 --> SW3            //
	////////////////////////////////////////////////////////////

	/// <summary>
	/// LinkTable filter query.
	/// Returns records where the Filter-field value matches the Filter-field value of the selected result-record.
	/// Provide a Link-Table Query, 
	/// a Results-Table Query to be filtered,
	/// the field_index in the Link-Table to select the first result record.
	/// the field_index in the Results-Table for filtering.
	/// </summary>	
	template <typename LinkRecordType, typename ReturnedRecordType>
	class QueryLinkF_T : public QueryF_T<ReturnedRecordType> {
		// all derived queries inherit from the root, rather from each other, so the virtual return-types can be derived-type.
	public:
		QueryLinkF_T(
			Query & link_query
			, Query & result_query
			, int select_f
			, int filter_f
		) : QueryF_T<ReturnedRecordType>(result_query, filter_f), _linkQuery(link_query, result_query, select_f) {}

		QueryLinkF_T(
			const TableQuery & link_query
			, const TableQuery & result_query
			, int select_f
			, int filter_f
		) : QueryF_T<ReturnedRecordType>(result_query, filter_f), _linkQuery(link_query, result_query, select_f) {}

		RecordSelector begin() override {
			auto rs = RecordSelector{ *this, incrementQ().begin() };
			getMatch(rs, 1, matchArg());
			return rs;
		}

		void setMatchArg(int matchArg) override {
			auto resultRec = _linkQuery[matchArg];
			QueryF_T<ReturnedRecordType>::acceptMatch(RecordSelector{ *this,resultRec });
		}

		RecordSelector end() override { return { *this, incrementQ().end() }; }
		
		RecordSelector last() override {
			auto match = RecordSelector{ *this, incrementQ().last() };
			if (match.status() == TB_OK) getMatch(match, -1, matchArg());
			return { match };
		}

		void acceptMatch(RecordSelector & recSel) override {
			incrementQ().acceptMatch(recSel);
		}

		void next(RecordSelector & recSel, int moveBy) override { 
			QueryF_T<ReturnedRecordType>::next(recSel, moveBy);
		}

	private:

		Answer_Locator getMatch(RecordSelector & recSel, int direction, int matchArg) override {
			QueryF_T<ReturnedRecordType>::getMatch(recSel, direction, matchArg);
			return recSel;
		}

		QueryL_T<LinkRecordType, ReturnedRecordType> _linkQuery;
	};

	////////////////////////////////////////////////////
	//            Inner-Join Query                    //
	//________________________________________________//
	// JoinTable, join field {1}     ResultsTable     //
	//                {0}  {1}          {0} {1}       //
	// match(0)-->[0] fred, 1---\    [0] 1, SW1       //
	//            [1] john, 0    \-->[1] 0, SW2 ->SW2 //
	////////////////////////////////////////////////////

	/// <summary>
	/// Inner-Join query.
	/// Select the unique record from the Results table, using Join-Table select-field,
	/// where the Match argument is the ID of the Join-Table record.
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
		) : CustomQuery(join_query), _result_query(&result_query), _select_f(select_f) {}

		QueryIJ_T(
			const TableQuery & join_query,
			const TableQuery & result_query,
			int select_f
		) : CustomQuery(join_query), _resultsTableQ(result_query), _result_query(&_resultsTableQ), _select_f(select_f) {}
		
		Query & resultsQ() override { return *_result_query; }
		Answer_Locator operator[](int index) override { setMatchArg(index); return Query::operator[](index); }
		
		void acceptMatch(RecordSelector & recSel) override {
			setMatchArg(recSel.id());
		}

		void next(RecordSelector & recSel, int moveBy) override { setMatchArg(matchArg() + moveBy); Query::next(recSel, moveBy); }
		bool uniqueMatch() override { return true; }

	private:
		Answer_Locator getMatch(RecordSelector & recSel, int, int matchArg) {
			if (matchArg != recSel.id()) {
				recSel.query().incrementTableQ().moveTo(recSel, matchArg);
			}
			Answer_R<JoinRecordType> select = recSel.incrementRecord();
			if (select.status() == TB_OK) {
				auto resultRS = _result_query->end();
				resultRS.setID(select.field(_select_f));
				_result_query->acceptMatch(resultRS);
				return (*_result_query)[select.field(_select_f)];
			}
			else return _result_query->end();
		}

		TableQuery _resultsTableQ;
		Query * _result_query;
		int _select_f; // Field in Join-Table used to select Results
	};
	
}