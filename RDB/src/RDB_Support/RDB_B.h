#pragma once
#include "RDB_Capacities.h"
#include <Arduino.h>


namespace RelationalDatabase {
	
	inline bool isSorted(InsertionStrategy strategy) {
		return (strategy > i_reverseOrder);
	}

	inline bool isSmallestFirst(InsertionStrategy strategy) {
		return (strategy < i_90_randomInsert);
	}

	class TableNavigator;
	class Table;
	template <typename Record_T> class RecordSelector_T;

	//#include "RDB_Query.h" // created circular header dependancy
	//class Query; // convenience for users of RDB.

	/// <summary>
	/// The Base-class for templated RDB.
	/// RDB_B can be built on any memory by providing read/write function in RDB_B constructor
	/// Tables are unsorted by default, but various sort options are available - see ChunkHeader Class.
	/// Allows Tables to grow beyond their initial declared capacity by automatically adding further Chunks.
	/// Multiple Tables can be created, but they cannot be deleted.
	/// RecordID's are unique and stable within an unsorted table and can be used for creating relationships
	/// The only db compaction is on deletion of the only record in a chunk; if it is the last chunk in the db, all final empty chunks are removed.
	/// </summary>
	class RDB_B {
	public:
		enum DB_Status : uint8_t {
			DB_OK,
			DB_LOAD_ERROR,
			DB_TABLE_INDEX_OUT_OF_RANGE,
			DB_FULL
		};

		template <typename Record_T> class Table_T;

		typedef int WriteByte_Handler(int address, const void * data, int noOfBytes);
		typedef int ReadByte_Handler(int address, void * result, int noOfBytes);

		RDB_B(uint16_t dbStartAddr, uint16_t dbMaxAddr, WriteByte_Handler *, ReadByte_Handler *, size_t password);
		RDB_B(uint16_t dbStartAddr, WriteByte_Handler *, ReadByte_Handler *, size_t password);

		// Queries
		bool passwordOK() const { return _dbEndAddr != 0; }

		// Modifiers
		virtual void reset(size_t password, uint16_t dbMaxAddr);
		int getTables(Table * table, int maxNoOfTables);
		int moveRecords(int fromAddress, int toAddress, int noOfBytes);

		Table createTable(size_t recsize, int initialNoOfRecords, InsertionStrategy strategy);
		bool extendTable(TableNavigator & rec_sel);
	private:
		friend class Answer_Locator;
		friend class TableNavigator;
		friend class Table;
		//template <typename Record_T> friend class RecordSelector_T;
		// Queries
		static ValidRecord_t unvacantRecords(int noOfRecords);
		bool checkPW(size_t password) const;
		// Modifiers
		void setDB_Header();
		void updateDB_Header() { _writeByte(_dbStartAddr+ SIZE_OF_PASSWORD, &_dbEndAddr, sizeof(DB_Size_t)); }
		void loadDB_Header();
		void savePW(size_t password);

		// Data
		WriteByte_Handler *_writeByte = 0;
		ReadByte_Handler *_readByte = 0;
		DB_Size_t _dbStartAddr = 0; // EEPROM address of start of database
		// The next items are the DB Header, saved in the DB
		// the password
		DB_Size_t _dbEndAddr = 0; // Current DB end addr
		DB_Size_t _dbMaxAddr = 0; // EEPROM address of end of avaialble space
		// End of Header
		enum { DB_HeaderSize = SIZE_OF_PASSWORD + sizeof(DB_Size_t) + sizeof(DB_Size_t), ValidRecord_t_Capacity = sizeof(ValidRecord_t) * 8 };
	};

}