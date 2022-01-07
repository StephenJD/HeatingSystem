#include "RDB_B.h"
#include "RDB_Table.h"
#include "RDB_TableNavigator.h"
#include <Logging.h>
using namespace arduino_logger;

namespace RelationalDatabase {
	RDB_B::RDB_B(uint16_t dbStartAddr, uint16_t dbMaxAddr, WriteByte_Handler * w, ReadByte_Handler * r, size_t password )
		: _writeByte(w),
		_readByte(r),
		_dbStartAddr(dbStartAddr),
		_dbEndAddr(dbStartAddr + DB_HeaderSize),
		_dbMaxAddr(dbMaxAddr)
	{
		savePW_OK(password);
		setDB_Header_OK();
		logger() << F("Construct RDB_B. Create new") << L_endl;
	}

	RDB_B::RDB_B(uint16_t dbStartAddr, WriteByte_Handler * w, ReadByte_Handler * r, size_t password )
		: _writeByte(w),
		_readByte(r),
		_dbStartAddr(dbStartAddr)
	{
		if (passWord_OK(password)) {
			loadDB_Header_OK();
			logger() << F("Construct RDB_B. Load Header. DBSize: ") << _dbEndAddr << L_endl;
		}
		else {
			logger() << F("Construct RDB_B. Password mismatch") << L_endl;
		}
	}

	bool RDB_B::reset_OK(size_t password, uint16_t dbMaxAddr) {
		auto status = savePW_OK(password);
		if (status) {
			_dbEndAddr = _dbStartAddr + DB_HeaderSize;
			_dbMaxAddr = dbMaxAddr;
			status = setDB_Header_OK();
		}
		return status;
	}

	bool RDB_B::savePW_OK(size_t password) {
		return _writeByte(_dbStartAddr, &password, SIZE_OF_PASSWORD);
	}

	bool RDB_B::passWord_OK(size_t password) const {
		size_t pw = 0;
		auto status = _readByte(_dbStartAddr, &pw, SIZE_OF_PASSWORD);
		logger() << F("PW was ") << pw << F(" Should be ") << password << L_endl;
		if (status && pw == password) {
			const_cast<RDB_B *>(this)->_dbEndAddr = _dbStartAddr + DB_HeaderSize; // _dbEndAddr != 0 indicates PW was correct.
			return true;
		} else {
			const_cast<RDB_B*>(this)->_dbEndAddr = 0;
			return false;
		}
	}

	bool RDB_B::setDB_Header_OK() {
		auto addr = _writeByte(_dbStartAddr + SIZE_OF_PASSWORD, &_dbEndAddr, sizeof(DB_Size_t)); // _dbEndAddr is next address available for a new table-header
		if(addr) addr = _writeByte(addr, &_dbMaxAddr, sizeof(DB_Size_t));
		return addr > 0;
	}

	bool RDB_B::loadDB_Header_OK() {
		DB_Size_t addr = _dbStartAddr + SIZE_OF_PASSWORD;
		addr = _readByte(addr, &_dbEndAddr, sizeof(DB_Size_t));
		if (addr) addr = _readByte(addr, &_dbMaxAddr, sizeof(DB_Size_t));
		return addr > 0;
	}

	Table RDB_B::createTable(size_t recsize, int initialNoOfRecords, InsertionStrategy strategy) {
		int extraValidRecordBytes = TableNavigator::validRecordByteNo(initialNoOfRecords- 1);
		DB_Size_t table_size = Table::HeaderSize + extraValidRecordBytes + (initialNoOfRecords * static_cast<DB_Size_t>(recsize));
		// address of table start and Table_Header to return
		TableID tableID = 0;
		ChunkHeader th;

		// Check available space
		if (_dbEndAddr + table_size <= _dbMaxAddr) {
			tableID = _dbEndAddr;
			DB_Size_t addr = _dbEndAddr;
			th.rec_size(static_cast<Record_Size_t>(recsize));
			th.finalChunk(true);
			th.chunkSize(initialNoOfRecords);
			th.insertionStrategy(strategy);
			th.validRecords(unvacantRecords(initialNoOfRecords));	// set all bits to "Used"
			addr = _writeByte(addr, &th, Table::HeaderSize);
			for (int i = 0; i < extraValidRecordBytes; ++i) {
				if (addr == 0) break;
				// unset bits up to initialNoOfRecords
				initialNoOfRecords = initialNoOfRecords - (sizeof(ValidRecord_t) << 3);
				ValidRecord_t usedRecords = unvacantRecords(initialNoOfRecords);
				addr = _writeByte(addr, &usedRecords, 1);
			}
			_dbEndAddr += table_size;
			updateDB_Header();
			//logger() << F("RDB_B::createTable() ExtraVR's: ") << extraValidRecordBytes << L_endl;
			//logger() << F("RDB_B::createTable() IsFinalChunk: ") << th.isFinalChunk() << L_endl;
			//logger() << F("RDB_B::createTable() NextChunk(not valid if final): ") << th.nextChunk() << L_endl;
			if (addr == 0) {
				logger() << F("RDB_B::createTable() Failed to write") << L_endl;
				return { *this, 0, th };
			}
		}
		return { *this, tableID, th };
	}

	ValidRecord_t RDB_B::unvacantRecords(int noOfRecords) { // static
		// Bitshifting more than the width is undefined
		if (noOfRecords >= ValidRecord_t_Capacity) return 0;
		return ValidRecord_t(-1) << noOfRecords;
	}

	bool RDB_B::extendTable(TableNavigator & rec_sel) {
		Table extension{ createTable(rec_sel.recordSize(), rec_sel.chunkCapacity(), i_retainOrder) };
		TableNavigator newChunk = &extension;
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
			return true;
		}
		return false;
	}

	int RDB_B::getTables(Table * tableArr, int maxNoOfTables) {
		//logger() << F(" RDB_B::getTables 1st Table addr: ") << int(_dbStartAddr + DB_HeaderSize) << L_endl;
		Table table(*this, _dbStartAddr + DB_HeaderSize);
		int i;
		for (i = 0; i < maxNoOfTables && table.isOpen(); ++i, ++tableArr) {
			//logger() << F(" RDB_B::gotTable : ") << i << L_endl;
			*tableArr = table;
			table.openNextTable();
		}
		logger() << F(" RDB_B::getTables loaded ") << i << F(" tables.") << L_endl;
		return i;
	}

	bool RDB_B::moveRecords_OK(int fromAddress, int toAddress, int noOfBytes) {
		while (noOfBytes > 0) {
			uint8_t readValue;
			fromAddress = _readByte(fromAddress, &readValue, 1);
			toAddress = _writeByte(toAddress, &readValue, 1);
			if (fromAddress == 0 || toAddress == 0) {
				fromAddress = 0;
				break;
			}
			--noOfBytes;
		}
		return fromAddress > 0;
	}

}