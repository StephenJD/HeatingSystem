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
		TB_Status readRecord(RecordID recordID, void * result) const;
		bool isOpen() const { return _rec_size != 0; }
		//TableNavigator begin() const;

		TableID tableID() const { return _tableID; }
		RDB_B & db() const { return *_db; }
		InsertionStrategy insertionStrategy() const {return _insertionStrategy;}
		bool outOfDate(uint32_t lastRead) const {
			//if (debugStop) logger() << F("outOfDate offered: ") << L_dec << lastRead << F(" timeOfLastChange was: ") << _timeOfLastChange << F(" Returned: ") << (_timeOfLastChange >= lastRead) << L_endl;
			return int32_t(_timeOfLastChange - lastRead) > 0;
		}
		uint32_t lastModifiedTime() const { return _timeOfLastChange; }
		NoOf_Recs_t maxRecordsInTable() const { return _max_NoOfRecords_in_table; }
		// Modifiers
		void openTable();
		void openNextTable();
		//void refresh();
	protected:

	private:
		friend TableNavigator;
		friend class RDB_B;
		friend class Answer_Locator;
		template<int maxNoOfTables> friend class RDB;

		enum { HeaderSize = sizeof(ChunkHeader), ValidRecordStart = offsetof(ChunkHeader, _validRecords), NoUnusedRecords = -1 };
		Table() = default;
		Table(const Table & rhs) = default; // prevent client table copying, as update-time on one will not be seen on the other
		Table & operator=(const Table & rhs) = default;

		/// <summary>	
		/// For loading an existing table
		/// </summary>	
		Table(RDB_B & db, TableID tableID);

		//Queries
		const ChunkHeader & table_header() const { return _table_header; }
		NoOf_Recs_t chunkSize() const { return _rec_size * _recordsPerChunk; }
		Record_Size_t recordSize() const { return _rec_size; } // return type must be same as sizeof()
		NoOf_Recs_t maxRecordsInChunk() const { return _recordsPerChunk; }
		bool dbInvalid() const { return _db == 0; }
		// Modifiers
		bool calcRecordSizeOK(TableNavigator rec_sel);
		void loadHeader(TableID _chunkAddr, ChunkHeader & _chunk_header) const;
		void markAsInvalid() { _rec_size = 0; }
		void tableIsChanged(bool reloadHeader);

		// Data
		mutable uint32_t _timeOfLastChange = 0;
		RDB_B * _db = 0;
		ChunkHeader _table_header;		// Header of First Chunk
		NoOf_Recs_t _recordsPerChunk = 0;		  // Copy of maxRecords from last Chunk
		TableID _tableID = 0;				// Address of first chunk
		Record_Size_t _rec_size = 0;			  // Copy of rec_size from last Chunk
		InsertionStrategy _insertionStrategy = i_retainOrder; // Copy from last Chunk
		NoOf_Recs_t _max_NoOfRecords_in_table = 0;
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
