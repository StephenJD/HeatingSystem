#include "RDB_TableQuery.h"
//#include "RDB_Table.h"
#include "Arduino.h"
#include <ostream>
#include <bitset>
using namespace std;



namespace RelationalDatabase {


	////////////////////////////////////////////////////////////////////////////
	// *************************** TableQuery *************************************//
	////////////////////////////////////////////////////////////////////////////

	TableQuery::TableQuery() { _currRecord.status(TB_INVALID_TABLE); }

	TableQuery::TableQuery(const RDB_B::Table & t) :
		_chunk_header(t.table_header()),
		_chunkAddr(t.tableID())
	{};

	TableQuery::TableQuery(RDB_B::Table & t) :
		_t(&t),
		_chunk_header(t.table_header()),
		_chunkAddr(t.tableID())
	{};

	// *************** Insert, Update & Delete ***************

	RecordID TableQuery::insertRecord(const void * newRecord) {
		auto targetRecordID = _currRecord.id();
		auto unusedRecordID = reserveUnusedRecordID();
		if (isSorted(_t->insertionStrategy()) && unusedRecordID < targetRecordID) {
			shuffleRecordsBack(unusedRecordID, targetRecordID);
			targetRecordID = unusedRecordID;
			unusedRecordID = reserveUnusedRecordID();
		} else targetRecordID = unusedRecordID;
		_currRecord.id(unusedRecordID);
		if (_currRecord.id() >= _t->_max_NoOfRecords_in_table) {
			TableQuery rec_sel = _t->_db->extendTable(*this);
			if (rec_sel._chunkAddr != 0) {
				haveMovedToNextChunck();
			}
			else {
				_currRecord.status(TB_TABLE_FULL);
				return _currRecord.id();
			}
		}
		_t->_db->_writeByte(recordAddress(), newRecord, _t->recordSize());
		_currRecord.status(TB_OK);
		_t->tableIsChanged(_chunk_header.isFirstChunk());
		return targetRecordID;
	}

	void TableQuery::shuffleRecordsBack(RecordID start, RecordID end) {
		// lambda
		auto inThisVRbyte = [this](auto currVRbyte_addr) -> bool {
			return getAvailabilityByteAddress() == currVRbyte_addr;
		};
		--end;
		// algorithm
		moveToThisRecord(start);
		while (start < end) {
			auto currVRbyte_addr = getAvailabilityByteAddress();
			while (start < end && inThisVRbyte(currVRbyte_addr)) {
				auto writeAddress = recordAddress();
				++start;
				moveToThisRecord(start); // move to possibly unused record
				auto readAddress = recordAddress();
				db().moveRecords(readAddress, writeAddress, recordSize());
			}
			shuffleValidRecordsByte(currVRbyte_addr, start==end || status() == TB_RECORD_UNUSED);
		}
	}

	NoOf_Recs_t TableQuery::thisVRcapacity() const {
		//auto chunkStart = (_currRecord._id / _t->maxRecordsInChunk()) * _t->maxRecordsInChunk();
		auto vrStartIndex = ((_currRecord._id - _chunk_start_recordID) / RDB_B::ValidRecord_t_Capacity) * RDB_B::ValidRecord_t_Capacity;
		auto maxVRIndex = _t->maxRecordsInChunk() + _chunk_start_recordID - vrStartIndex;
		auto maxIndex = _t->maxRecordsInTable() - vrStartIndex;
		if (maxIndex < RDB_B::ValidRecord_t_Capacity) return maxIndex; // where last chunk has smaller capacity than previous chunks
		else if (_t->maxRecordsInChunk() < RDB_B::ValidRecord_t_Capacity) return _t->maxRecordsInChunk(); // where chunk capacity is less than a VR
		else if (maxVRIndex < RDB_B::ValidRecord_t_Capacity) return maxVRIndex; // where follow-on VR has reduced capacity
		else return RDB_B::ValidRecord_t_Capacity;
	}

	void TableQuery::shuffleValidRecordsByte(TB_Size_t availabilityByteAddr, bool shiftIn_VacantRecord) {
		ValidRecord_t usedRecords;
		db()._readByte(availabilityByteAddr, &usedRecords, sizeof(ValidRecord_t));
		usedRecords >>= 1;
		constexpr ValidRecord_t topBit = 1 << (RDB_B::ValidRecord_t_Capacity - 1);
		usedRecords |= topBit;
		if (shiftIn_VacantRecord) {
			ValidRecord_t maxBit = 1 << (thisVRcapacity()-1);
			usedRecords &= ~maxBit;
		}
		db()._writeByte(availabilityByteAddr, &usedRecords, sizeof(ValidRecord_t));
		_t->tableIsChanged(_chunk_header.isFirstChunk());
		//cout << " shuffled VR Byte at: " << dec << (int)availabilityByteAddr << " is: " << bitset<8>(usedRecords) << endl;
	}

	//RecordID TableQuery::insertRecords(const void * record, int noOfRecords) {
	//	const uint8_t * addr = static_cast<const uint8_t *>(record);
	//	RecordID recordID = 0;
	//	for (int i = 0; i < noOfRecords; ++i) {
	//		recordID = insertRecord(addr);
	//		addr += recordSize();
	//	}
	//	return recordID;
	//}

	// *************** Iterator Functions ***************

	TableQuery & TableQuery::operator+=(TableQuery offset) {
		int direction = (offset._currRecord.signed_id() < 0 ? -1 : 1);
		_currRecord._id += offset._currRecord.signed_id();
		moveToNextUsedRecord(direction);
		return *this;
	}

	TableQuery & TableQuery::operator-=(TableQuery offset) {
		int direction = (offset._currRecord.signed_id() < 0 ? 1 : -1);
		_currRecord._id -= offset._currRecord.signed_id();
		moveToNextUsedRecord(direction);
		return *this;
	}

	TableQuery & TableQuery::begin() {
		if (moveToFirstChunck()) moveToNextUsedRecord(1);
		return *this;
	}

	TableQuery & TableQuery::end() {
		moveToThisRecord(endStopID());
		return *this;
	}

	// ********  First Used Record ***************
	void TableQuery::moveToNextUsedRecord(int direction) {
		auto currID = _currRecord._id; // +direction;
		moveToThisRecord(_currRecord.id());
		if (_currRecord.status() == TB_BEFORE_BEGIN) {
		}
		else {
			bool found = moveToUsedRecordInThisChunk(direction);
			while (!found && haveMovedToNextChunck()) {
				found = moveToUsedRecordInThisChunk(direction);
			}
			if (!found) {
				if (_currRecord.id() == RecordID(-1)) {
					_currRecord.status(TB_BEFORE_BEGIN);
				}
				else {
					_currRecord.id(endStopID());
					_currRecord.status(TB_END_STOP);
				}
			}
		}
	}

	bool TableQuery::moveToUsedRecordInThisChunk(int direction) {
		uint8_t vrIndex;
		ValidRecord_t usedRecords;
		bool withinChunk = true;
		do getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
		while (!haveMovedToUsedRecord(usedRecords, vrIndex, direction) && (withinChunk = (_currRecord.id() - _chunk_start_recordID < _t->maxRecordsInChunk())));
		return withinChunk;
	}

	bool TableQuery::haveMovedToUsedRecord(ValidRecord_t usedRecords, unsigned char vrIndex, int direction) {
		if (usedRecords == 0) {
			_currRecord._id += maxRecords() < RDB_B::ValidRecord_t_Capacity ? maxRecords() : RDB_B::ValidRecord_t_Capacity;
			if (_currRecord._id > _t->maxRecordsInTable()) _currRecord._id = _t->maxRecordsInTable();
			return false;
		}
		ValidRecord_t recordMask = 1 << vrIndex;
		if (usedRecords != ValidRecord_t(-1)) {
			while (recordMask && (usedRecords & recordMask) == 0) {
				recordMask <<= 1;
				_currRecord._id += direction;
			}
		}

		if (recordMask == 0 || _currRecord._id - _chunk_start_recordID >= _t->maxRecordsInChunk())
			return false;
		else
			_currRecord.status(TB_OK);
			return true;
	}

	// ******** Obtain Unused Record for Insert Record ***************
	// ********      Implements insert strategy        ***************

	RecordID TableQuery::reserveUnusedRecordID() {
		bool found = reserveFirstUnusedRecordInThisChunk();
		if (!found && _chunkAddr != _t->tableID()) {
			moveToFirstChunck();
			found = reserveFirstUnusedRecordInThisChunk();
		}
		while (!found && haveMovedToNextChunck()) {
			found = reserveFirstUnusedRecordInThisChunk();
		}
		return _currRecord.id();
	}

	bool TableQuery::reserveFirstUnusedRecordInThisChunk() {
		// Look in current ValidRecordByte, move _currRecord.id to unused record, or one-past this chunk
		bool gotUnusedRecord;
		RecordID unusedRecID = _currRecord.id();
		ValidRecord_t usedRecords;
		uint8_t vrIndex;
		TB_Size_t vr_Byte_Addr = getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
		if (_currRecord.status() == TB_RECORD_UNUSED) {
			usedRecords |= (1 << vrIndex);
			gotUnusedRecord = true;
		} else {
			_currRecord.id(_chunk_start_recordID);
			unusedRecID = _chunk_start_recordID + _VR_Byte_No * RDB_B::ValidRecord_t_Capacity;
			gotUnusedRecord = haveReservedUnusedRecord(usedRecords, unusedRecID);
			if (!gotUnusedRecord) {
				if (_VR_Byte_No != 0) {
					// Then search this chunk from the beginning
					_currRecord.id(_chunk_start_recordID);
					vr_Byte_Addr = getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
					gotUnusedRecord = haveReservedUnusedRecord(usedRecords, unusedRecID);
				}
			}
			while (!gotUnusedRecord && _VR_Byte_No < noOfvalidRecordBytes()) {
				_currRecord._id += thisVRcapacity(); // to move VR-byte on
				vr_Byte_Addr = getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
				unusedRecID = _currRecord.id();
				gotUnusedRecord = haveReservedUnusedRecord(usedRecords, unusedRecID);
			}
		}
		if (gotUnusedRecord) {
			if (_VR_Byte_No == 0) {
				_chunk_header.validRecords(usedRecords);
				saveHeader();
				if (_t->_tableID == _chunkAddr) _t->_table_header.validRecords(usedRecords);
			}
			else {
				db()._writeByte(vr_Byte_Addr, &usedRecords, sizeof(ValidRecord_t));
			}
		}
		_currRecord.id(unusedRecID);
		return gotUnusedRecord;
	}

	bool TableQuery::haveReservedUnusedRecord(ValidRecord_t & usedRecords, RecordID & unusedRecID) const {
		// assumes unusedRecID is ValidRecord_start_ID
		if (usedRecords == ValidRecord_t(-1)) {
			unusedRecID += thisVRcapacity();
			return false;
		}
		else {
			ValidRecord_t recordMask = 1;
			while ((usedRecords & recordMask) > 0) {
				recordMask <<= 1;
				++unusedRecID;
			}
			// Set bit to 1
			usedRecords |= recordMask;
			return true;
		}
	}

	// ********  Last Used Record for end() ***************

	bool TableQuery::getLastUsedRecordInThisChunk(RecordID & usedRecID) {
		ValidRecord_t usedRecords = getFirstValidRecordByte();
		DB_Size_t endaddr = firstRecordInChunk();
		DB_Size_t chunkAddr = _chunkAddr + RDB_B::Table::HeaderSize;
		RecordID vr_start = 0;
		bool found = false;
		do {
			found |= lastUsedRecord(usedRecords, usedRecID, vr_start);
		} while (endaddr > chunkAddr && (chunkAddr = db()._readByte(chunkAddr, &usedRecords, sizeof(ValidRecord_t)))); // assignment intended
		return found;
	}

	bool TableQuery::lastUsedRecord(ValidRecord_t usedRecords, RecordID & usedRecID, RecordID & vr_start) const {
		ValidRecord_t increment = maxRecords() - vr_start;
		increment = increment < RDB_B::ValidRecord_t_Capacity ? increment : RDB_B::ValidRecord_t_Capacity;
		vr_start += increment;
		if (usedRecords == 0) return false;
		RecordID found = _chunk_start_recordID + vr_start;
		ValidRecord_t recordMask = 1 << (increment - 1);
		--found;
		if (usedRecords != ValidRecord_t(-1)) {
			while (recordMask && (usedRecords & recordMask) == 0) {
				recordMask >>= 1;
				--found;
			}
		}
		if (recordMask) {
			usedRecID = found;
			return true;
		}
		else return false;
	}

	// ********  Extension ***************

	void TableQuery::extendChunkTo(TableID nextChunk) {
		_chunk_header.finalChunk(0);
		_chunk_header.nextChunk(nextChunk);
		if (_t->tableID() == _chunkAddr) _t->_table_header = _chunk_header;
		saveHeader();
	}

	// ********  Navigation Support ***************

	void TableQuery::checkStatus() {
		RecordID vrIndex;
		ValidRecord_t usedRecords;
		getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
		if (!recordIsUsed(usedRecords, vrIndex)) _currRecord.status(TB_RECORD_UNUSED);
		else _currRecord.status(TB_OK);
	}

	bool TableQuery::moveToFirstChunck() {
		if (tableInvalid()) return false;
		_chunkAddr = _t->tableID();
		_chunk_header = _t->table_header();
		_chunk_start_recordID = 0;
		_currRecord.id(0);
		checkStatus();
		return true;
	}

	bool TableQuery::haveMovedToNextChunck() {
		if (chunkIsExtended()) {
			_chunk_start_recordID += _t->maxRecordsInChunk();
			_chunkAddr = _chunk_header.nextChunk();
			_t->loadHeader(_chunkAddr, _chunk_header);
			_VR_Byte_No = 0;
			checkStatus();
			return true;
		}
		else return false;
	}

	void TableQuery::moveToThisRecord(int recordID) {
		if (recordID == RecordID(-1)) {
			moveToFirstChunck();
			recordID = -1;
			_currRecord.status(TB_BEFORE_BEGIN);
		} else if (recordID >= _t->maxRecordsInTable()) {
			_currRecord.status(/*recordID == _t->maxRecordsInTable() ?*/ TB_END_STOP /*: TB_BEFORE_BEGIN*/);
			recordID = _t->maxRecordsInTable();
		}
		else {
			if (recordID < _chunk_start_recordID) moveToFirstChunck();
			while (recordID - _chunk_start_recordID >= _t->maxRecordsInChunk() && haveMovedToNextChunck()) {;}
			_currRecord.id(recordID);
			checkStatus();
		}
		_currRecord.id(recordID);
	}



	/*TB_Status TableQuery::saveRecord(const void * record) {
		TB_Status status = TB_OK;
		RecordID vrIndex;
		ValidRecord_t usedRecords;
		TB_Size_t availabilityByteAddr = getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
		if (recordIsUsed(usedRecords, vrIndex)) {
			usedRecords &= ~(1 << vrIndex);
			if (_VR_Byte_No == 0) {
				_chunk_header.validRecords(usedRecords);
				if (_t->_tableID == _chunkAddr) _t->_table_header.validRecords(usedRecords);
				saveHeader();
			}
			else {
				db()._writeByte(availabilityByteAddr, &usedRecords, sizeof(ValidRecord_t));
			}
			db()._writeByte(recordAddress(), record, recordSize());
			_t->tableIsChanged(_chunk_header.isFirstChunk());
		}
		else {
			status = TB_RECORD_UNUSED;
		}
		return status;
	}*/

	TB_Size_t  TableQuery::getAvailabilityByteAddress() const {
		if (_VR_Byte_No == 0) {
			return _chunkAddr + RDB_B::Table::ValidRecordStart;
		}
		else {
			return _chunkAddr + RDB_B::Table::HeaderSize + (_VR_Byte_No - 1) * sizeof(ValidRecord_t);
		}
	}

	uint8_t TableQuery::getValidRecordIndex() const {
		uint8_t vrIndex = _currRecord.id() - _chunk_start_recordID;
		auto vr_byteNo = validRecordByteNo(vrIndex);
		if (noOfvalidRecordBytes() >= vr_byteNo) {
			_VR_Byte_No = vr_byteNo;
		}
		if (_VR_Byte_No > 0) vrIndex = vrIndex % RDB_B::ValidRecord_t_Capacity;
		return vrIndex;
	}

	TB_Size_t  TableQuery::getAvailabilityBytesForThisRecord(ValidRecord_t & usedRecords, unsigned char & vrIndex) const {
		// Assumes we are in the correct chunk. Returns the availabilityByteAddr or 0 for byte(0)
		uint8_t old_VR_Byte_No = _VR_Byte_No;
		vrIndex = getValidRecordIndex();
		TB_Size_t availabilityByteAddr = getAvailabilityByteAddress();
		//if (debugStop) {
		//	debugStop = false;
		//}
		if (old_VR_Byte_No != _VR_Byte_No || _t->outOfDate(_timeValidRecordLastRead)) {
			if (_VR_Byte_No == 0) {
				const_cast<TableQuery *>(this)->loadHeader(); // invalidated by moving to another chunk or by table-update
				usedRecords = _chunk_header.validRecords();
			}
			else {
				db()._readByte(availabilityByteAddr, &usedRecords, sizeof(ValidRecord_t)); // invalidated by moving to another ValidRecord_byte or by table-update
				_timeValidRecordLastRead = millis();
			}
			const_cast<TableQuery *>(this)->_chunk_header.validRecords(usedRecords);
		}
		else {
			usedRecords = _chunk_header.validRecords();
		}
		return availabilityByteAddr;
	}

	bool TableQuery::isDeletedRecord() {
		RecordID vrIndex;
		ValidRecord_t usedRecord;
		getAvailabilityBytesForThisRecord(usedRecord, vrIndex);
		if (recordIsUsed(usedRecord, vrIndex)) {
			_currRecord.status(TB_OK);
			return false;
		}
		else {
			_currRecord.status(TB_RECORD_UNUSED);
			return true;
		}
	}

	bool TableQuery::recordIsUsed(ValidRecord_t usedRecords, int vrIndex) const {
		// Bitshifting more than the width is undefined
		if (vrIndex >= RDB_B::ValidRecord_t_Capacity) return false;
		return ((usedRecords & (1 << vrIndex)) > 0);
	};

	DB_Size_t TableQuery::recordAddress() const {
		return firstRecordInChunk() + (_currRecord.id() - _chunk_start_recordID)  * _t->recordSize();
	}

	void TableQuery::loadHeader() {
		db()._readByte(_chunkAddr, &_chunk_header, RDB_B::Table::HeaderSize);
		_timeValidRecordLastRead = millis();
	}

	void TableQuery::saveHeader() {
		db()._writeByte(_chunkAddr, &_chunk_header, RDB_B::Table::HeaderSize);
	}

	ValidRecord_t TableQuery::getFirstValidRecordByte() const {
		if (_t->outOfDate(_timeValidRecordLastRead)) {
			const_cast<TableQuery *>(this)->loadHeader(); // invalidated by moving to another chunk or by table-update
		}
		return _chunk_header.validRecords();
	}

	RecordID TableQuery::sortedInsert(void * recToInsert
		, bool(*compareRecords)(TableQuery * left, const void * right, bool lhs_IsGreater)
		, void(*swapRecords)(TableQuery *original, void * recToInsert)) {
		
		//cout << "  Curr RecordID " << dec << (int)_currRecord.signed_id() << status() << endl;
		if (status() == TB_RECORD_UNUSED) ++(*this);
		bool sortOrder = isSmallestFirst(_t->insertionStrategy());
		bool moveToNext = compareRecords(this, recToInsert, sortOrder);
		bool needToMove = true;
		if (status() == TB_END_STOP) {
			if (_currRecord.id() == 0) {
				needToMove = false;
				moveToNext = true;
				_currRecord.id(RecordID(-1));
			} else {
				--(*this);
				moveToNext = compareRecords(this, recToInsert, sortOrder);
			}
		}
		int moveDirection = moveToNext ? -1 : 1;
		// Move to insertion point
		int insertionPos = _currRecord.signed_id();
		while (needToMove) {
			insertionPos = _currRecord.signed_id();
			(*this) += moveDirection; // to next valid record
			auto thisStatus = status();
			if (thisStatus == TB_BEFORE_BEGIN || status() == TB_END_STOP) break;
			needToMove = (compareRecords(this, recToInsert, sortOrder) == moveToNext);
		}
		if (moveDirection == -1) {
			insertionPos = _currRecord.signed_id();
		} 

		++insertionPos;

		// Find Space
		moveToThisRecord(insertionPos);
		//cout << "      InsertPos " << dec << (int)insertionPos << status() << endl;
		ValidRecord_t validRecords;
		unsigned char vrIndex;
		getAvailabilityBytesForThisRecord(validRecords, vrIndex);
		auto insertPosTaken = recordIsUsed(validRecords, vrIndex);
		//cout << "      validRecords " << std::bitset<8>(validRecords) << endl;

		// Shuffle Records
		if (insertPosTaken) {
			auto swapPos = insertionPos;
			if (status() == TB_BEFORE_BEGIN) status(TB_OK); // only if -1
			while (insertPosTaken && status() == TB_OK) {
				swapRecords(this, recToInsert);
				++swapPos;
				moveToThisRecord(swapPos);
				getAvailabilityBytesForThisRecord(validRecords, vrIndex);
				insertPosTaken = recordIsUsed(validRecords, vrIndex);
			}
		}
		return insertionPos;
	}

	////////////////////////////////////////////////////////////////////////////
	// *************************** Answer_Hoist *************************************//
	////////////////////////////////////////////////////////////////////////////

	Answer_Hoist::Answer_Hoist(TableQuery & rs)
		: Answer(rs.get().id(), rs.get().status()),
		_lastRead(0),
		_tb(&rs.table())
	{
		if (_tb) {
			_recordAddress = rs.recordAddress();
			_validRecordByteAddress = rs.getAvailabilityByteAddress();
			_validRecordIndex = rs.getValidRecordIndex();
		}
	}

	Answer_Hoist & Answer_Hoist::operator = (const TableQuery & rs) {
		Answer::id(rs.get().id());
		Answer::status(rs.get().status());
		if (rs.status() != TB_INVALID_TABLE) {
			_lastRead = 0;
			_recordAddress = rs.recordAddress();
			_tb = &const_cast<TableQuery &>(rs).table();
			_validRecordByteAddress = rs.getAvailabilityByteAddress();
			_validRecordIndex = rs.getValidRecordIndex();
		}
		return *this;
	}


	void Answer_Hoist::rec(void * rec) {
		if (_tb->outOfDate(_lastRead)) {
			statusOnly();
			if (_status == TB_OK) _tb->db()._readByte(_recordAddress, rec, _tb->recordSize());
			_lastRead = millis();
		}
	}

	TB_Status Answer_Hoist::status(void * thisRec) {
		if (_status != TB_INVALID_TABLE) rec(thisRec);
		return _status;
	}

	TB_Status Answer_Hoist::statusOnly() {
		if (_tb->outOfDate(_lastRead)) {
			if (_id < _tb->maxRecordsInTable()) {
				ValidRecord_t usedRecords;
				_tb->db()._readByte(_validRecordByteAddress, &usedRecords, sizeof(ValidRecord_t));
				if (usedRecords & (1 << _validRecordIndex)) {
					_status = TB_OK;
				}
				else if (_status != TB_END_STOP) { _status = TB_RECORD_UNUSED; }
			}
		}
		return _status;
	}

	void Answer_Hoist::deleteRecord() {
		ValidRecord_t usedRecords;
		_tb->db()._readByte(_validRecordByteAddress, &usedRecords, sizeof(ValidRecord_t));
		usedRecords &= ~(1 << _validRecordIndex);
		_tb->db()._writeByte(_validRecordByteAddress, &usedRecords, sizeof(ValidRecord_t));
		_lastRead = millis();
		_tb->tableIsChanged(true);
		_status = TB_RECORD_UNUSED;
	}

	void Answer_Hoist::update(const void * rec) {
		if (statusOnly() == TB_OK) {
			_tb->db()._writeByte(_recordAddress, rec, _tb->recordSize());
			_lastRead = millis();
			_tb->tableIsChanged(false);
		}
	}
}
