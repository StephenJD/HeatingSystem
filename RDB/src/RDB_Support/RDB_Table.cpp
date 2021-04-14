#include "RDB_Table.h"
#include "RDB_TableNavigator.h"
#include "..\..\..\Logging\Logging.h"

//extern bool debugStop;
using namespace arduino_logger;

namespace RelationalDatabase {
	/////////////////////////////////////////////////////////////////////
	// ******************* Table Functions *************************** //
	/////////////////////////////////////////////////////////////////////

	Table::Table(RDB_B & db, TableID tableID, ChunkHeader & tableHdr)
		// used when creating a new table
		: _db(&db)
		, _table_header(tableHdr)
		, _tableID(tableID)
		, _recordsPerChunk(tableHdr.chunkSize())
		, _rec_size(tableHdr.rec_size())
		, _insertionStrategy(tableHdr.insertionStrategy())
		, _max_NoOfRecords_in_table(_recordsPerChunk)
	{
	}

	Table::Table(RDB_B & db, TableID tableID) :
		// used when loading an existing table
		_db(&db),
		_tableID(tableID)
	{
		openTable();
	}

	void Table::openNextTable() {
		TableNavigator rec_sel(this);

		do { // Move to next chunk in the DB - might be the extention to an earlier table.			
			rec_sel._chunkAddr = rec_sel.firstRecordInChunk() + chunkSize();
			
			auto tableOK = (rec_sel._chunkAddr > _db->_dbStartAddr) && (rec_sel._chunkAddr < _db->_dbEndAddr);
			
			if (tableOK) {
				_db->_readByte(rec_sel._chunkAddr, &rec_sel._chunk_header, HeaderSize);
				//logger() << F("Table::openNextTable() chunk at: ") << rec_sel._chunkAddr << L_endl;
			}
			else {
				//logger() << F("Table::openNextTable() no more Tables") << L_endl;
				markAsInvalid();
				return;
			}

			tableOK = calcRecordSizeOK(rec_sel);

			if (tableOK) {
				auto areMoreChuncks = rec_sel.chunkIsExtended();
				if (areMoreChuncks) {
					auto nextChunkAddr = rec_sel._chunk_header.nextChunk();
					//logger() << F(" NextChunk addr: ") << nextChunkAddr << L_endl;
					if (nextChunkAddr == rec_sel._chunkAddr || nextChunkAddr > _db->_dbEndAddr) {
						tableOK = false;
					}
				}
			} else logger() << F("calcRecordSize failed: ") << _max_NoOfRecords_in_table << L_endl;

			if (!tableOK) {
				logger() << F(" Corrupt Table found. Database Reset.") << L_endl;
				_db->reset(0,0);
				markAsInvalid();
				return;
			}
		} while (!rec_sel.isStartOfATable());
		_tableID = rec_sel._chunkAddr;
		_table_header = rec_sel._chunk_header;
	}

	void Table::openTable() {
		if (_tableID) {
			loadHeader(_tableID, _table_header);
			//logger() << "Open T. Calc RecSize" << L_endl;
			calcRecordSizeOK(this);
		}
	}

	bool Table::calcRecordSizeOK(TableNavigator rec_sel) {
		_recordsPerChunk = rec_sel._chunk_header.chunkSize();
		
		//logger() << F(" calcRecordSizeOK. recPerCh: ") << _recordsPerChunk << F(" Rec Size: ") << rec_sel._chunk_header.rec_size();
		//logger() << F("\n   Chunk Addr: ") << rec_sel._chunkAddr << L_endl;
		//if (!rec_sel._chunk_header.isFinalChunk()) logger() << F(" NextChunk at: ") << rec_sel._chunk_header.nextChunk() << L_endl;
		
		_max_NoOfRecords_in_table = _recordsPerChunk;
		
		if (_recordsPerChunk == 0) return false;

		while (rec_sel.haveMovedToNextChunck()) {
			_max_NoOfRecords_in_table += _recordsPerChunk;
			//logger() << F("     _max_NoOfRecords_in_table: ") << _max_NoOfRecords_in_table << L_endl;
		}
		_rec_size = rec_sel._chunk_header.rec_size();
		_insertionStrategy = rec_sel._chunk_header.insertionStrategy();
		return true;
	}

	void Table::loadHeader(TableID _chunkAddr, ChunkHeader & _chunk_header) const {
		_db->_readByte(_chunkAddr, &_chunk_header, HeaderSize);
	}

	TB_Status Table::readRecord(RecordID recordID, void * result) const {
		TableNavigator rec_sel(const_cast<Table *>(this));
//		logger() << F("readRecord : ") << recordID << L_endl;
		rec_sel.moveToThisRecord(recordID);
//		logger() << F("readRecord moved ") << L_endl;
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

	uint8_t Table::tableIsChanged(bool reloadHeader) {
		if (reloadHeader) {
			loadHeader(_tableID, _table_header);
		}
		++_hdrVersionNo;
		return ++_vrVersionNo;
	}

	//TableNavigator Table::begin() const { return *this; }

}
