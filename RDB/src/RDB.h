#pragma once
#include "RDB_Support/RDB_Query.h"
#include "RDB_Support/RDB_TableNavigator.h"
#include "RDB_Support/RDB_Table.h"
#include "RDB_Support/RDB_B.h"
#include <Arduino.h>

namespace RelationalDatabase {

	template< class T > struct remove_const { typedef T type; };
	template< class T > struct remove_const<const T> { typedef T type; };

	template<int maxNoOfTables>
	class RDB : public RDB_B {
	public:
		/// <summary>
		/// For creating a new Database
		/// </summary>	
		RDB(int dbStart, int dbEnd, WriteByte_Handler * wbh, ReadByte_Handler * rbh) : RDB_B(dbStart, dbEnd, wbh, rbh) {};

		/// <summary>
		/// For opening an existing Database
		/// </summary>	
		RDB(int dbStart, WriteByte_Handler * wbh, ReadByte_Handler * rbh) : RDB_B(dbStart, wbh, rbh) {
			getTables(tables, maxNoOfTables);
		}

		template <typename Record_T>
		const TableQuery createTable(int initialNoOfRecords, InsertionStrategy strategy = i_retainOrder) {
			return registerTable(RDB_B::createTable(sizeof(Record_T), initialNoOfRecords, strategy));
		}

		template <typename Record_T>
		const TableQuery createTable(const Record_T & rec, int initialNoOfRecords, InsertionStrategy strategy = i_retainOrder) {
			TableQuery tq = registerTable(RDB_B::createTable(sizeof(Record_T), initialNoOfRecords, strategy));
			tq.insert(&rec);
			return tq;
		}

		template<typename Record_T, int noOfRecords>
		const TableQuery createTable(Record_T(&array)[noOfRecords], InsertionStrategy strategy = i_retainOrder) {
			TableQuery newTable = registerTable(RDB_B::createTable(sizeof(array[0]), noOfRecords, strategy));;
			newTable.insert(array, noOfRecords);
			return newTable;
		}

		Table table(int table_index) { return tables[table_index]; }

		TableQuery tableQuery(int table_index) {
			if (table_index >= 0 && table_index < maxNoOfTables) {
				return tables[table_index];
			}
			else {
				return {};
			}
		}

	private:
		Table & registerTable(const Table & t) {
			for (int i = 0; i < maxNoOfTables; ++i) {
				if (tables[i]._tableID == 0) { tables[i] = t; return tables[i]; }
			}
			return tables[0];
		}

		Table tables[maxNoOfTables];
	};

}