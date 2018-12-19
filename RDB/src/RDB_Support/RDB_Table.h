#pragma once
#include "RDB_ChunkHeader.h"
#include "RDB_B.h"
#include "RDB_Capacities.h"

extern bool debugStop;
namespace RelationalDatabase {

	/// <summary>
	/// Holds data common to all chunks in the table
	/// Similar concept to a Deque. Can be dynamically extended without invalidating its iterators.
	/// Record Access is via the TableNavigator which is an iterator.
	/// </summary>
	class Table {
	public:
		Table(RDB_B & db, TableID tableID, ChunkHeader & tableHdr);
		
		// Queries
		//TableNavigator begin() const;

		TableID tableID() const { return _tableID; }
		RDB_B & db() const { return *_db; }
		InsertionStrategy insertionStrategy() const {return _insertionStrategy;}
		void openTable();
		void openNextTable();
		bool outOfDate(unsigned long lastRead) {
			//if (debugStop) std::cout << "outOfDate offered: " << std::dec << lastRead << " timeOfLastChange was: " << _timeOfLastChange <<  " Returned: " << (_timeOfLastChange >= lastRead) << std::endl;
			return (_timeOfLastChange >= lastRead);
		}
		NoOf_Recs_t maxRecordsInTable() const { return _max_NoOfRecords_in_table; }

	protected:
		TB_Status readRecord(RecordID recordID, void * result) const;

	private:
		friend TableNavigator;
		friend class RDB_B;
		friend class Answer_Locator;
		template<int maxNoOfTables> friend class RDB;

		enum { HeaderSize = sizeof(ChunkHeader), ValidRecordStart = offsetof(ChunkHeader, _validRecords), NoUnusedRecords = -1 };
		Table() : _timeOfLastChange(0), _db(0), _tableID(0), _recordsPerChunk(0), _rec_size(0), _max_NoOfRecords_in_table(0) {};


		/// <summary>	
		/// For loading an existing table
		/// </summary>	
		Table(RDB_B & db, TableID tableID);

		//Queries
		const ChunkHeader & table_header() const { return _table_header; }
		NoOf_Recs_t chunkSize() const { return _rec_size * _recordsPerChunk; }
		bool isOpen() const { return _rec_size != 0; }
		Record_Size_t recordSize() const { return _rec_size; } // return type must be same as sizeof()
		NoOf_Recs_t maxRecordsInChunk() const { return _recordsPerChunk; }
		bool dbInvalid() const { return _db == 0; }
		// Modifiers
		bool getRecordSize(TableNavigator rec_sel);
		void loadHeader(TableID _chunkAddr, ChunkHeader & _chunk_header) const;
		void markAsInvalid() { _rec_size = 0; }
		void tableIsChanged(bool reloadHeader);

		// Data
		mutable unsigned long _timeOfLastChange;
		RDB_B * _db;
		ChunkHeader _table_header;		// Header of First Chunk
		TableID _tableID;				// Address of first chunk
		NoOf_Recs_t _recordsPerChunk;		  // Copy of maxRecords from last Chunk
		Record_Size_t _rec_size;			  // Copy of rec_size from last Chunk
		InsertionStrategy _insertionStrategy; // Copy from last Chunk
		NoOf_Recs_t _max_NoOfRecords_in_table;
	};

	template <typename Record_T>
	class  RDB_B::Table_T : public Table {
	public:
		Table_T(RDB_B * db, TableID tableID, ChunkHeader & tableHdr) : Table(*db, tableID, tableHdr) {}
		Table_T(const Table & t) : Table(t) {}

		Record_T readRecord(RecordID recordID) const {
			Record_T result = { 0 };
			Table::readRecord(recordID, &result);
			return result;
		}
	};

}
