#pragma once
//#include "RDB_B.h"
#include "RDB_Capacities.h"
#include "..\..\..\Bitfield\Bitfield.h"

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
		InsertionStrategy insertionStrategy() const { return static_cast<InsertionStrategy>(TableID(_insertionStrategy)); }
		//bool insertionOrderNeedsModifying() const { return _insertionOrderNeedsModifying == 1; }	// 0: Insertion order matches table Order (or doesn't matter) 1: Insertion order is reverse of Table Order (or random)
		//bool insertionsAreMostlyOrdered() const { return _insertionsAreMostlyOrdered == 1; }	// 1: Insertions take place from either end, with shuffles for unordered inserts. 0 : Insertions are in random order.
		//bool orderedByLargestFirst() const { return _orderedByLargestFirst == 1; }			// 0 : orderedBySmallestFirst

		NoOf_Recs_t chunkSize() const { return _chunkSize; }
		bool isFirstChunk() const { return _isExtendingChunk == 0; }
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

		void firstChunk(bool set) { _isExtendingChunk = !set; }
		void chunkSize(uint8_t size) { _chunkSize = size; }
	private:
		friend class Table;
		// Only or Final Chunk:
		//  { _recSize(bytes) : bottom 12-bits
		//    _insertionStrategy : 3-bits
		//    _isFinalChunk : top-bit = 1;
		//    _chunkSize(NoOfRecords) : 7-bits
		//	  _isExtendingChunk : top bit = 0/1;
		//    _validRecords : 1 or more bytes
		//  }
		//
		// Extended Chunk:
		//  { _nextChunk : bottom 15-bits
		//    _isFinalChunk : top-bit = 0;
		//    _chunkSize : 7-bits
		//	  _isExtendingChunk : top bit = 0/1; (0 == first chunk)
		//    _validRecords : 1 or more bytes
		//  } 

		enum {TableID_Size = sizeof(TableID) * 8, NoOfRecs_Size = sizeof(NoOf_Recs_t) * 8};
		union { // If top-bit is set, this is final chunk and this data is interpreted as Record-Size and Ordering flags
			TableID _next_chunk; // Address in EEPROM
			UBitField<TableID, 0, TableID_Size - 1> _nextChunk; // 15-bits address
			UBitField<TableID, TableID_Size - 1, 1> _isFinalChunk; // top-bit 
			UBitField<TableID, 0, TableID_Size - 4> _recSize; // or 12-bits = bytes per record
			UBitField<TableID, TableID_Size - 4, 3> _insertionStrategy; // 3-bits strategy _insertionOrderNeedsModifying;
			//UBitField<TableID, TableID_Size - 3, 1> _insertionsAreMostlyOrdered;
			//UBitField<TableID, TableID_Size - 2, 1> _orderedByLargestFirst;
		};
		union {
			NoOf_Recs_t _chunk_Size;	// If top-bit is clear, this is the first chunk. The rest is the chunkSize in noOfRecords (0-127). Required in every chunk for getRecordSize().
			UBitField<NoOf_Recs_t, 0, NoOfRecs_Size - 1> _chunkSize; // noOfRecords per chunck
			UBitField<NoOf_Recs_t, NoOfRecs_Size - 1, 1> _isExtendingChunk;
		};
		ValidRecord_t _validRecords; // bit-pattern of valid records in this chunk.
	};
}
