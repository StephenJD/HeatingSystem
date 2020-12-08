#pragma once
#include "RDB_ChunkHeader.h"
#include "RDB_B.h"
#include "RDB_Capacities.h"
#include <Logging.h>

extern bool debugStop;
namespace RelationalDatabase {
#ifdef ZPSIM
	constexpr uint16_t TIMER_INCREMENT = 1;
#else
	constexpr uint16_t TIMER_INCREMENT = 1;
#endif
	/// <summary>
	/// 17Bytes. Holds data common to all chunks in the table
	/// Similar concept to a Deque. Can be dynamically extended without invalidating its iterators.
	/// Record Access is via the TableNavigator which is an iterator.
	/// </summary>
	class Table {
	public:
		enum { HeaderSize = sizeof(ChunkHeader), ValidRecordStart = offsetof(ChunkHeader, _validRecords), NoUnusedRecords = -1 };
		Table(RDB_B & db, TableID tableID, ChunkHeader & tableHdr);
		// Queries
		TB_Status readRecord(RecordID recordID, void * result) const;
		bool isOpen() const { return _rec_size != 0; }
		//TableNavigator begin() const;

		TableID tableID() const { return _tableID; }
		RDB_B & db() const { return *_db; }
		InsertionStrategy insertionStrategy() const {return _insertionStrategy;}
		bool vrOutOfDate(uint8_t lastRead) const {
			//if (debugStop) logger() << F("vrOutOfDate offered: ") << L_dec << lastRead << F(" timeOfLastChange was: ") << vrVersion() << F(" Returned: ") << (vrVersion() >= lastRead) << L_endl;
			//auto result = int32_t(vrVersion() - lastRead);
			//if (result > 0) {
			//	logger() << F("vrOutOfDate offered: ") << L_dec << lastRead << F(" timeOfLastChange was: ") << vrVersion() << F(" Returned: ") << int32_t(_tabtableVersion()leVersionNo - lastRead) << L_endl;
			//}
			return int8_t(vrVersion() - lastRead) > 0;
		}

		bool hdrOutOfDate(uint8_t lastRead) const {
			return int8_t(hdrVersion() - lastRead) > 0;
		}

		uint8_t vrVersion() const { return _vrVersionNo; }
		uint8_t hdrVersion() const { return _hdrVersionNo; }
		NoOf_Recs_t maxRecordsInTable() const { return _max_NoOfRecords_in_table; }
		// Modifiers
		void openTable();
		void openNextTable();
		//void refresh();
		uint8_t tableIsChanged(bool reloadHeader);
		uint8_t vrByteIsChanged() { return ++_vrVersionNo;	}
		uint8_t dataIsChanged() { return ++_vrVersionNo;	}
		Record_Size_t recordSize() const { return _rec_size; } // return type must be same as sizeof()
		const ChunkHeader & table_header() const { return _table_header; }
		ChunkHeader & table_header() { return _table_header; }
		bool dbValid() const { return _db != 0; }
		NoOf_Recs_t maxRecordsInChunk() const { return _recordsPerChunk; }
		void loadHeader(TableID _chunkAddr, ChunkHeader & _chunk_header) const;
	protected:

	private:
		friend class RDB_B;
		template<int maxNoOfTables> friend class RDB;

		Table() = default;
		Table(const Table & rhs) = default; // prevent client table copying, as update-time on one will not be seen on the other
		Table & operator=(const Table & rhs) = default;

		/// <summary>	
		/// For loading an existing table
		/// </summary>	
		Table(RDB_B & db, TableID tableID);

		//Queries
		NoOf_Recs_t chunkSize() const { return _rec_size * _recordsPerChunk; }
		// Modifiers
		bool calcRecordSizeOK(TableNavigator rec_sel);
		void markAsInvalid() { _rec_size = 0; }
		// Data
		RDB_B * _db = 0;				// 4B
		ChunkHeader _table_header;		// 4B Header of First Chunk - Table VR is ALWAYS the first VR, whereas Record-Selector Header has current VR 
		TableID _tableID = 0;			// 2B Address of first chunk
		Record_Size_t _rec_size = 0;	// 2B Copy of rec_size from last Chunk
		NoOf_Recs_t _recordsPerChunk = 0; // 1B Copy of maxRecords from last Chunk
		InsertionStrategy _insertionStrategy = i_retainOrder; // 1B Copy from last Chunk
		NoOf_Recs_t _max_NoOfRecords_in_table = 0; // 1B
		mutable uint8_t _vrVersionNo = 0; // 1B
		mutable uint8_t _hdrVersionNo = 0; // 1B
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
