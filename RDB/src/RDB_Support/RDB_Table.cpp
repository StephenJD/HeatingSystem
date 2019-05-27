#include "RDB_Table.h"
#include "RDB_TableNavigator.h"
#include "Arduino.h" // for micros()
#include "..\..\..\Logging\Logging.h"

//extern bool debugStop;

namespace RelationalDatabase {
	/////////////////////////////////////////////////////////////////////
	// ******************* Table Functions *************************** //
	/////////////////////////////////////////////////////////////////////

	Table::Table(RDB_B & db, TableID tableID, ChunkHeader & tableHdr)
		// used when creating a new table
		:_timeOfLastChange(micros())
		, _db(&db)
		, _table_header(tableHdr)
		, _tableID(tableID)
		, _recordsPerChunk(tableHdr.chunkSize())
		, _rec_size(tableHdr.rec_size())
		, _insertionStrategy(tableHdr.insertionStrategy())
		, _max_NoOfRecords_in_table(_recordsPerChunk)
	{
		//logger().log("Table::Table _insertionStrategy:", _insertionStrategy);
	}

	Table::Table(RDB_B & db, TableID tableID) :
		// used when loading an existing table
		_timeOfLastChange(micros()),
		_db(&db),
		_tableID(tableID)
	{
		openTable();
	}

	Table::Table(const Table & rhs) :_timeOfLastChange(micros())
	{
		//logger().log("Table::Table copy  _insertionStrategy:", _insertionStrategy);
	}

	//Table & Table::operator =(const Table & rhs) {
	//	_timeOfLastChange = micros();
	//	_db = rhs._db;
	//	_table_header = rhs._table_header;
	//	_tableID = rhs._tableID;
	//	_recordsPerChunk = rhs._recordsPerChunk;
	//	_rec_size = rhs._rec_size;
	//	_insertionStrategy = rhs._insertionStrategy;
	//	_max_NoOfRecords_in_table = rhs._max_NoOfRecords_in_table;
	//	//logger().log("Table::Table assignment  _insertionStrategy:", _insertionStrategy);
	//	return *this;
	//}


	void  Table::openNextTable() {
		TableNavigator rec_sel(this);

		do { // Move to next chunk in the DB - might be the extention to an earlier table.
			rec_sel._chunkAddr = rec_sel.firstRecordInChunk() + chunkSize();
			if (rec_sel._chunkAddr < _db->_dbSize) {
				_db->_readByte(rec_sel._chunkAddr, &rec_sel._chunk_header, HeaderSize);
				logger().log("Table::openNextTable() chunk at:", rec_sel._chunkAddr);
			}
			else {
				logger().log("Table::openNextTable() no more Tables");
				markAsInvalid();
				return;
			}
			if (!rec_sel._chunk_header.isFinalChunk() && (rec_sel._chunk_header.nextChunk() == rec_sel._chunkAddr || rec_sel._chunk_header.nextChunk() > _db->_dbSize)) {
				//logger().log("Table::openNextTable() read from", rec_sel._chunkAddr, "Size is :", _db->_dbSize);
				//logger().log(" A table-extention? :", !rec_sel.isStartOfATable(), "Chunk _isFinalChunk", rec_sel._chunk_header.isFinalChunk());
				//logger().log(" if Final: chunk _recSize", rec_sel._chunk_header._recSize, " ELSE NextChunk :", rec_sel._chunk_header.nextChunk());
				//logger().log(" IsfirstChunk :", rec_sel._chunk_header.isFirstChunk(), "  chunk noOfRecords", rec_sel._chunk_header.chunkSize());
				logger().log(" Corrupt Table found. Database Reset.");
				_db->reset(0,0);
				markAsInvalid();
				return;
			}
		} while (!rec_sel.isStartOfATable() && getRecordSize(rec_sel));
		_tableID = rec_sel._chunkAddr;
		_table_header = rec_sel._chunk_header;
		getRecordSize(rec_sel);
	}

	void Table::openTable() {
		if (_tableID) {
			loadHeader(_tableID, _table_header);
			getRecordSize(this);
		}
	}

	bool Table::getRecordSize(TableNavigator rec_sel) {

		_recordsPerChunk = rec_sel._chunk_header.chunkSize();
		_max_NoOfRecords_in_table = _recordsPerChunk;
		logger().log(" getRecordSize. recPerCh:", _recordsPerChunk, "Rec Size:", rec_sel._chunk_header.rec_size());
		logger().log("   Addr:", rec_sel._chunkAddr, "NextChunk at:", rec_sel._chunk_header.nextChunk());

		while (rec_sel.haveMovedToNextChunck()) {
			_max_NoOfRecords_in_table += _recordsPerChunk;
			logger().log("     _max_NoOfRecords_in_table:", _max_NoOfRecords_in_table);
		}
		_rec_size = rec_sel._chunk_header.rec_size();
		_insertionStrategy = rec_sel._chunk_header.insertionStrategy();
		//logger().log("Table::getRecordSize _insertionStrategy:", _insertionStrategy);
		return true;
	}

	void Table::loadHeader(TableID _chunkAddr, ChunkHeader & _chunk_header) const {
		_db->_readByte(_chunkAddr, &_chunk_header, HeaderSize);
	}

	TB_Status Table::readRecord(RecordID recordID, void * result) const {
		TableNavigator rec_sel(const_cast<Table *>(this));
		rec_sel.moveToThisRecord(recordID);
		if (rec_sel._currRecord.status() == TB_OK) {
			if (rec_sel.isDeletedRecord()) {
				rec_sel._currRecord.setStatus(TB_RECORD_UNUSED);
			}
			else {
				db()._readByte(rec_sel.recordAddress(), result, recordSize());
			}
		}
		return rec_sel._currRecord.status();
	}

	void Table::tableIsChanged(bool reloadHeader) {
		_timeOfLastChange = micros();
		//if (debugStop) std::cout << "tableIsChanged: " << std::dec << _timeOfLastChange << std::endl;
		if (reloadHeader) {
			loadHeader(_tableID, _table_header);
		}
	}

	//TableNavigator Table::begin() const { return *this; }

}
