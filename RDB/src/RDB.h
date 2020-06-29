#pragma once
#include "RDB_Support/RDB_Query.h"
#include "RDB_Support/RDB_TableNavigator.h"
#include "RDB_Support/RDB_Table.h"
#include "RDB_Support/RDB_B.h"
#include "RDB_Support/RDB_Capacities.h"
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
		RDB(int dbStartAddr, int dbMaxAddr, WriteByte_Handler * wbh, ReadByte_Handler * rbh, size_t password) : RDB_B(dbStartAddr, dbMaxAddr, wbh, rbh, password) {};

		/// <summary>
		/// For opening an existing Database
		/// </summary>	
		RDB(int dbStartAddr, WriteByte_Handler * wbh, ReadByte_Handler * rbh, size_t password) : RDB_B(dbStartAddr, wbh, rbh, password) {
			if (checkPW(password)) {
				getTables(tables, maxNoOfTables);
			}
		}

		void reset(size_t password, uint16_t dbMaxAddr) override {
			RDB_B::reset(password, dbMaxAddr);
			for (auto & table : tables) table = Table{};
		}

		template <typename Record_T>
		const TableQuery createTable(int initialNoOfRecords, InsertionStrategy strategy = i_retainOrder) {
			return registerTable(RDB_B::createTable(sizeof(Record_T), initialNoOfRecords, strategy));
		}

		template <typename Record_T>
		const TableQuery createTable(const Record_T & rec, int initialNoOfRecords, InsertionStrategy strategy = i_retainOrder) {
			TableQuery tq = registerTable(RDB_B::createTable(sizeof(Record_T), initialNoOfRecords, strategy));
			tq.insert(rec);
			return tq;
		}

		template<typename Record_T, int noOfRecords>
		const TableQuery createTable(Record_T(&array)[noOfRecords], InsertionStrategy strategy = i_retainOrder) {
			//auto & table = RDB_B::createTable(sizeof(array[0]), noOfRecords, strategy);
			auto & tableR = registerTable(RDB_B::createTable(sizeof(array[0]), noOfRecords, strategy));

			TableQuery newTable = tableR;
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
		Table * begin() { return tables; }
		Table * end() { return tables + maxNoOfTables; }

	private:
		Table & registerTable(const Table & t) {
			for (auto & tbl : tables) {
				if (tbl._tableID == t._tableID) {
					logger() << L_tabs << F(" Registered Table found ID:") << t._tableID << F(" pos") << &tbl - tables << L_endl;
					return tbl;
				}
				else if (tbl._tableID == 0) {
					logger()  << L_tabs << F(" Register Table ID: ") << t._tableID << F(" pos") << &tbl - tables << L_endl;
					tbl = t; 
					return tbl; 
				}
			}
			return tables[maxNoOfTables];
		}

		Table tables[maxNoOfTables+1];
	};

}