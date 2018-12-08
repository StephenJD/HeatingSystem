#include "RDB_B.h"
#include "RDB_Table.h"
#include "RDB_TableNavigator.h"

namespace RelationalDatabase {
	RDB_B::RDB_B(int dbStart, int dbEnd, WriteByte_Handler * w, ReadByte_Handler * r)
		: _writeByte(w),
		_readByte(r),
		_dbStart(dbStart),
		_dbSize(dbStart),
		_dbEnd(dbEnd)
	{
		setDB_Header();
		Serial.println("RDB_B(create) Constructed");

	}

	RDB_B::RDB_B(int dbStart, WriteByte_Handler * w, ReadByte_Handler * r)
		: _writeByte(w),
		_readByte(r),
		_dbStart(dbStart)
	{
		loadDB_Header();
		Serial.println("RDB_B(load) Constructed");
	}

	void RDB_B::setDB_Header() {
		_dbSize = _writeByte(_dbStart, &_dbSize, sizeof(DB_Size_t));
		_dbSize = _writeByte(_dbSize, &_dbEnd, sizeof(DB_Size_t));
	}

	void RDB_B::loadDB_Header() {
		DB_Size_t addr = _dbStart;
		addr = _readByte(addr, &_dbSize, sizeof(DB_Size_t));
		addr = _readByte(addr, &_dbEnd, sizeof(DB_Size_t));
	}

	Table RDB_B::createTable(size_t recsize, int initialNoOfRecords, InsertionStrategy strategy) {
		int extraValidRecordBytes = TableNavigator::validRecordByteNo(initialNoOfRecords - 1);
		DB_Size_t table_size = Table::HeaderSize + extraValidRecordBytes + (initialNoOfRecords * static_cast<DB_Size_t>(recsize));
		// address of table start and Table_Header to return
		TableID tableID = 0;
		ChunkHeader th;

		// Check available space
		if (_dbSize + table_size <= _dbEnd) {
			tableID = _dbSize;
			DB_Size_t addr = _dbSize;
			th.rec_size(static_cast<Record_Size_t>(recsize));
			th.chunkSize(initialNoOfRecords);
			th.insertionStrategy(strategy);
			th.validRecords(unvacantRecords(initialNoOfRecords));	// set all bits to "Used"
			addr = _writeByte(addr, &th, Table::HeaderSize);
			for (int i = 0; i < extraValidRecordBytes; ++i) {
				// unset bits up to initialNoOfRecords
				initialNoOfRecords = initialNoOfRecords - (sizeof(ValidRecord_t) << 3);
				ValidRecord_t usedRecords = unvacantRecords(initialNoOfRecords);
				addr = _writeByte(addr, &usedRecords, 1);
			}
			_dbSize += table_size;
			updateDB_Header();
		}
		return Table(*this, tableID, th);
	}

	ValidRecord_t RDB_B::unvacantRecords(int noOfRecords) { // static
		// Bitshifting more than the width is undefined
		if (noOfRecords >= ValidRecord_t_Capacity) return 0;
		return -1 << noOfRecords;
	}

	TableNavigator RDB_B::extendTable(TableNavigator & rec_sel) {
		TableNavigator newChunk = &createTable(rec_sel.recordSize(), rec_sel.chunkCapacity(), i_retainOrder);
		if (newChunk._chunkAddr != 0) {
			// We only extend if we are trying to insert a record, so set first record-bit as used.
			newChunk._t = rec_sel._t;
			auto availableRecords = newChunk._chunk_header.validRecords();
			newChunk._chunk_header = rec_sel._chunk_header; // Copy final-chunk flags
			newChunk._chunk_header.validRecords(1 | availableRecords);
			newChunk._chunk_header.firstChunk(0);
			newChunk.saveHeader();
			rec_sel._t->_max_NoOfRecords_in_table += rec_sel.chunkCapacity();
			rec_sel.extendChunkTo(newChunk._chunkAddr);
		}
		return newChunk;
	}

	Table  RDB_B::getTable(int tablePosition) {
		Table table(*this, _dbStart + DB_HeaderSize);

		for (int t = 0; t < tablePosition; ++t) {
			table.openNextTable();
			if (!table.isOpen()) break;
		};
		return table;
	}

	int RDB_B::getTables(Table * tableArr, int maxNoOfTables) {
		Table table(*this, _dbStart + DB_HeaderSize);
		int i;
		for (i = 0; i < maxNoOfTables && table.isOpen(); ++i, ++tableArr) {
			*tableArr = table;
			table.openNextTable();
		}
		return i;
	}

	int RDB_B::moveRecords(int fromAddress, int toAddress, int noOfBytes) {
		while (noOfBytes > 0) {
			unsigned char readValue;
			_readByte(fromAddress, &readValue, 1);
			_writeByte(toAddress, &readValue, 1);
			++fromAddress;
			++toAddress;
			--noOfBytes;
		}
		return fromAddress;
	}

}