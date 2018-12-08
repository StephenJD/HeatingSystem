#pragma once
#include "RDB_B.h"
#include "RDB_Capacities.h"
#include <..\Bitfield\Bitfield.h>

extern bool debugStop;
namespace RelationalDatabase {
	using namespace BitFields;

	/// <summary>
	/// Gives the record-size(in bytes) and Chunk-size (no of records)
	/// Tables can be extended by chaining chunks together. New chunks have the same size as the first.
	/// For non-final chunks, next_chunk holds the address of the next chunk. 
	/// Has flags indicating the ordering of records and the insertion strategy to be used.
	/// The default is an unordered table.
	/// Ordered Tables reassign RecordID's during sorting.
	/// Setting "insertionOrderNeedsModifying" reverses the insertion order if "insertionsAreMostlyOrdered" is set, 
	///   or inserts mid-way between the next highest and lowest records in order (for random order inserts).
	/// Setting "insertionsAreMostlyOrdered" causes insertions to take place from one end with shuffles when required
	/// If "insertionsAreMostlyOrdered" is clear and "insertionOrderNeedsModifying" is clear, the table is unordered.
	/// Largest,Ord,ReverseInsertion
	/// 0,0,0 = accept insertion order
	/// 0,0,1 = reverse insertion order
	/// 1,0,0 = sort random insertions smallest first
	/// 1,0,1 = sort random insertions largest first
	/// 0,1,0 = sort smallest first, insertions mostly smallest first
	/// 1,1,0 = sort largest first, insertions mostly largest first
	/// 0,1,1 = sort smallest first, insertions mostly largest first
	/// 1,1,1 = sort largest first, insertions mostly smallest first
	/// </summary>
	struct ChunkHeader {

		ChunkHeader() : _next_chunk(0), _chunk_Size(0), _validRecords(0) {
			firstChunk(true);
			finalChunk(true);
		}

		// Queries
		TableID nextChunk() const { return _nextChunk; }
		bool isFinalChunk() const { return _isFinalChunk == 1; }
		Record_Size_t rec_size() const { return _recSize; }
		InsertionStrategy insertionStrategy() const { return InsertionStrategy(unsigned int(_insertionStrategy)); }
		//bool insertionOrderNeedsModifying() const { return _insertionOrderNeedsModifying == 1; }	// 0: Insertion order matches table Order (or doesn't matter) 1: Insertion order is reverse of Table Order (or random)
		//bool insertionsAreMostlyOrdered() const { return _insertionsAreMostlyOrdered == 1; }	// 1: Insertions take place from either end, with shuffles for unordered inserts. 0 : Insertions are in random order.
		//bool orderedByLargestFirst() const { return _orderedByLargestFirst == 1; }			// 0 : orderedBySmallestFirst

		NoOf_Recs_t chunkSize() const { return _chunkSize; }
		bool isFirstChunk() const { return _firstChunk == 0; }
		ValidRecord_t validRecords() const { return _validRecords; }

		// Modifiers
		void nextChunk(TableID addr) { _nextChunk = addr; }
		void finalChunk(bool set) { _isFinalChunk = set; }
		void rec_size(TableID size) { _recSize = size; }
		void insertionStrategy(InsertionStrategy strategy) { _insertionStrategy = strategy; }
		//void insertionOrderNeedsModifying(bool set) { _insertionOrderNeedsModifying = set; }
		//void insertionsAreMostlyOrdered(bool set) { _insertionsAreMostlyOrdered = set; }
		//void orderedByLargestFirst(bool set) { _orderedByLargestFirst = set; }
		void validRecords(ValidRecord_t vr) { _validRecords = vr; }

		void firstChunk(bool set) { _firstChunk = !set; }
		void chunkSize(unsigned char size) { _chunkSize = size; }
	private:
		friend class Table;
		union { // If top-bit is set, this is final chunk and this data is interpreted as Record-Size and Ordering flags
			TableID _next_chunk;
			UBitField<TableID, 0, sizeof(TableID) * 8 - 1> _nextChunk; // 7-bits address
			UBitField<TableID, sizeof(TableID) * 8 - 1, 1> _isFinalChunk; // top-bit 
			UBitField<TableID, 0, sizeof(TableID) * 8 - 4> _recSize; // or 4-bits noOfRecords per chunck
			UBitField<TableID, sizeof(TableID) * 8 - 4, 3> _insertionStrategy; // 3-bits strategy _insertionOrderNeedsModifying;
			//UBitField<TableID, sizeof(TableID) * 8 - 3, 1> _insertionsAreMostlyOrdered;
			//UBitField<TableID, sizeof(TableID) * 8 - 2, 1> _orderedByLargestFirst;
		};
		union {
			NoOf_Recs_t _chunk_Size;	// If top-bit is clear, this the first chunk. The rest is the chunkSize in noOfRecords (0-127). Required in every chunk for getRecordSize().
			UBitField<NoOf_Recs_t, 0, sizeof(NoOf_Recs_t) * 8 - 1> _chunkSize;
			UBitField<NoOf_Recs_t, sizeof(NoOf_Recs_t) * 8 - 1, 1> _firstChunk;
		};
		ValidRecord_t _validRecords; // bit-pattern of valid records in this chunk.
	};

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
