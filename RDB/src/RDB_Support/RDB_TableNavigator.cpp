#include "RDB_TableNavigator.h"
#include "Arduino.h"
#include "RDB_Table.h"
#include "RDB_B.h"

using namespace std;

namespace RelationalDatabase {

	////////////////////////////////////////////////////////////////////////////
	// *************************** TableNavigator *************************************//
	////////////////////////////////////////////////////////////////////////////

	TableNavigator::TableNavigator() : _currRecord{ 0, TB_INVALID_TABLE } {}

	TableNavigator::TableNavigator(Table * t) :
		 _t(t)
		, _currRecord {RecordID(-1), TB_BEFORE_BEGIN }
	{ 
		if (t) {
			_chunk_header = t->table_header();
			_chunkAddr = t->tableID();
			_timeValidRecordLastRead = t->lastModifiedTime();
		}
		else {
			logger() << "Invalid Table\n";
			_currRecord.setStatus(TB_INVALID_TABLE);
		}
	}

	TableNavigator::TableNavigator(const Answer_Locator & answerLoc) : 
		TableNavigator(answerLoc.table())
	{
		_currRecord = answerLoc;
		moveToThisRecord(id());
	}


	TableNavigator::TableNavigator(int id, TB_Status status) :
		_t(0)
		, _currRecord(id, status)
	{}

	bool TableNavigator::operator==(const TableNavigator & other) const { return _currRecord.id() == other._currRecord.id(); }
	bool TableNavigator::operator<(const TableNavigator & other) const { return _currRecord.id() < other._currRecord.id(); }
	TB_Status TableNavigator::status() const { return _currRecord.status(); }
	int TableNavigator::id() const { return status() == TB_BEFORE_BEGIN ? -1 : _currRecord.id(); }
	void TableNavigator::setStatus(TB_Status status) { _currRecord.setStatus(status); }
	void TableNavigator::setID(int id) { _currRecord.setID(id); }
	DB_Size_t TableNavigator::firstRecordInChunk() const { return _chunkAddr + Table::HeaderSize + noOfvalidRecordBytes(); }
	bool TableNavigator::tableValid() { return _t ? _t->dbValid() : false; }
	RDB_B & TableNavigator::db() const { return _t->db(); }
	Record_Size_t TableNavigator::recordSize() const { return _t->_rec_size; }
	NoOf_Recs_t TableNavigator::chunkCapacity() const { return _t->maxRecordsInChunk(); }
	NoOf_Recs_t TableNavigator::endStopID() const { return _t->maxRecordsInTable(); }

	// *************** Insert, Update & Delete ***************

	RecordID TableNavigator::insertRecord(const void * newRecord) {
		auto targetRecordID = _currRecord.id();
		auto unusedRecordID = reserveUnusedRecordID();
		//logger() << "Insert: " << *reinterpret_cast<const uint32_t*>(newRecord) << L_endl;
		if (isSorted(_t->insertionStrategy()) && unusedRecordID < targetRecordID) {
			shuffleRecordsBack(unusedRecordID, targetRecordID);
			targetRecordID = unusedRecordID;
			unusedRecordID = reserveUnusedRecordID();
			//logger() << F(" unusedRecordID: ") << (int)unusedRecordID << L_endl;

		} else targetRecordID = unusedRecordID;
		_currRecord.setID(unusedRecordID);
		if (_currRecord.id() >= _t->_max_NoOfRecords_in_table) {
			if (_t->_db->extendTable(*this)) {
				haveMovedToNextChunck();
			}
			else {
				_currRecord.setStatus(TB_TABLE_FULL);
				return _currRecord.id();
			}
		}
		_t->_db->_writeByte(recordAddress(), newRecord, _t->recordSize());
		_currRecord.setStatus(TB_OK);
		_t->tableIsChanged(_chunk_header.isFirstChunk());
		return targetRecordID;
	}

	void TableNavigator::shuffleRecordsBack(RecordID start, RecordID end) {
		// lambda
		auto inThisVRbyte = [this](TB_Size_t currVRbyte_addr) -> bool {
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

	NoOf_Recs_t TableNavigator::thisVRcapacity() const { // 1-8
		// for small chunks, VRcapacity and chunkCapacity will be less than 8!
		auto vrStartIndex = _VR_ByteNo * RDB_B::ValidRecord_t_Capacity; // 0,8,16
		auto maxVRIndex = chunkCapacity() - vrStartIndex; // within a chunk, startRecID might be 
		if (maxVRIndex < RDB_B::ValidRecord_t_Capacity) return maxVRIndex;
		else return RDB_B::ValidRecord_t_Capacity;
	}

	void TableNavigator::shuffleValidRecordsByte(TB_Size_t availabilityByteAddr, bool shiftIn_VacantRecord) {
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
	}

	// *************** Iterator Functions ***************

	void TableNavigator::moveToLastRecord() {
		moveToThisRecord(endStopID());
		moveToNextUsedRecord(-1);
	}	
	
	void TableNavigator::next(RecordSelector & recSel, int moveBy) {
		operator+=(moveBy);
	}
	
	TableNavigator & TableNavigator::operator+=(int offset) {
		int direction = (offset < 0 ? -1 : 1);
		_currRecord.setID(_currRecord.signed_id() + offset);
		moveToNextUsedRecord(direction);
		return *this;
	}

	// ********  First Used Record ***************
	void TableNavigator::moveToNextUsedRecord(int direction) {
		auto currID = _currRecord.signed_id();
		moveToThisRecord(_currRecord.signed_id());

		if (_currRecord.status() == TB_BEFORE_BEGIN) {
			if (direction > 0) {
				_currRecord.setID(0);
				moveToUsedRecordInThisChunk(direction);
			}
		}
		else {
			if (_currRecord.status() == TB_END_STOP && direction < 0 && _currRecord.id() > 0) moveToThisRecord(_currRecord.id()-1);
			bool found = moveToUsedRecordInThisChunk(direction);
			while (!found && _currRecord.status() != TB_END_STOP && _currRecord.signed_id() > 0) {
				moveToThisRecord(_currRecord.id());
				found = moveToUsedRecordInThisChunk(direction);
			}
			if (!found) {
				if (direction < 0 ) {
					_currRecord.setID(RecordID(-1));
					_currRecord.setStatus(TB_BEFORE_BEGIN);
				}
				else {
					_currRecord.setID(endStopID());
					_currRecord.setStatus(TB_END_STOP);
				}
			}
		}
	}

	bool TableNavigator::moveToUsedRecordInThisChunk(int direction) {
		auto isWithinChunk = [this]() {
			int indexInChunk = _currRecord.id() - _chunk_start_recordID;
			if (indexInChunk < 0) return false;
			else if (indexInChunk >= chunkCapacity()) return false;
			else return true;
		};

		uint8_t vrIndex;
		ValidRecord_t usedRecords;
		bool withinChunk = true;
		do getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
		while (!haveMovedToUsedRecord(usedRecords, vrIndex, direction) && (withinChunk = isWithinChunk()));
		return withinChunk;
	}

	bool TableNavigator::haveMovedToUsedRecord(ValidRecord_t usedRecords, uint8_t initialVRindex, int direction) {
		auto VRcapacity = thisVRcapacity();

		// Lambdas
		auto moveIDtoEndOfVRbyte = [direction, VRcapacity, this]() {
			_currRecord.setID(_currRecord.id() + (direction == -1 ? -VRcapacity : VRcapacity));
		};

		// Algorithm
		if (usedRecords == 0) {
			moveIDtoEndOfVRbyte();
			return false;
		}
		ValidRecord_t recordMask = 1 << initialVRindex;
		if (usedRecords != ValidRecord_t(-1)) {
			while (recordMask && (usedRecords & recordMask) == 0) {
				(direction > 0) ? recordMask <<= 1 : recordMask >>= 1;
				_currRecord.setID(_currRecord.id() + direction);
			}
		}

		if (recordMask == 0 || _currRecord.id() - _chunk_start_recordID >= chunkCapacity()) {
			if (_currRecord.id() >= endStopID()) _currRecord.setStatus(TB_END_STOP);
			return false;
		} else {
			_currRecord.setStatus(TB_OK);
			return true;
		}
	}

	// ******** Obtain Unused Record for Insert Record ***************
	// ********      Implements insert strategy        ***************

	RecordID TableNavigator::reserveUnusedRecordID() {
		bool found = reserveFirstUnusedRecordInThisChunk();
		if (!found && _chunkAddr != _t->tableID()) {
			moveToFirstRecord();
			found = reserveFirstUnusedRecordInThisChunk();
		}
		while (!found && haveMovedToNextChunck()) {
			found = reserveFirstUnusedRecordInThisChunk();
		}
		return _currRecord.id();
	}

	bool TableNavigator::reserveFirstUnusedRecordInThisChunk() {
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
			_currRecord.setID(_chunk_start_recordID);
			unusedRecID = _chunk_start_recordID + _VR_ByteNo * RDB_B::ValidRecord_t_Capacity;
			gotUnusedRecord = haveReservedUnusedRecord(usedRecords, unusedRecID);
			if (!gotUnusedRecord) {
				if (_VR_ByteNo != 0) {
					// Then search this chunk from the beginning
					_currRecord.setID(_chunk_start_recordID);
					vr_Byte_Addr = getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
					unusedRecID = _chunk_start_recordID;
					gotUnusedRecord = haveReservedUnusedRecord(usedRecords, unusedRecID);
				}
			}
			while (!gotUnusedRecord && _VR_ByteNo < noOfvalidRecordBytes()) {
				_currRecord.setID(_currRecord.id() + thisVRcapacity()); // to move VR-byte on
				vr_Byte_Addr = getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
				unusedRecID = _currRecord.id();
				gotUnusedRecord = haveReservedUnusedRecord(usedRecords, unusedRecID);
			}
		}
		if (gotUnusedRecord) {
			db()._writeByte(vr_Byte_Addr, &usedRecords, sizeof(ValidRecord_t));
		}
		_currRecord.setID(unusedRecID);
		return gotUnusedRecord;
	}

	bool TableNavigator::haveReservedUnusedRecord(ValidRecord_t & usedRecords, RecordID & unusedRecID) const {
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

	bool TableNavigator::getLastUsedRecordInThisChunk(RecordID & usedRecID) {
		ValidRecord_t usedRecords;
		DB_Size_t endaddr = firstRecordInChunk();
		DB_Size_t chunkAddr = _chunkAddr + Table::HeaderSize - 1;
		RecordID vr_start = 0;
		bool found = false;
		do {
			chunkAddr = db()._readByte(chunkAddr, &usedRecords, sizeof(ValidRecord_t));
			found |= lastUsedRecord(usedRecords, usedRecID, vr_start);
		} while (endaddr > chunkAddr);
		return found;
	}

	bool TableNavigator::lastUsedRecord(ValidRecord_t usedRecords, RecordID & usedRecID, RecordID & vr_start) const {
		ValidRecord_t increment = chunkCapacity() - vr_start;
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

	void TableNavigator::extendChunkTo(TableID nextChunk) {
		_chunk_header.finalChunk(0);
		_chunk_header.nextChunk(nextChunk);
		if (_t->tableID() == _chunkAddr) _t->_table_header = _chunk_header;
		saveHeader();
	}

	// ********  Navigation Support ***************

	void TableNavigator::checkStatus() {
		RecordID vrIndex;
		ValidRecord_t usedRecords;
		getAvailabilityBytesForThisRecord(usedRecords, vrIndex);
		if (!recordIsUsed(usedRecords, vrIndex)) _currRecord.setStatus(TB_RECORD_UNUSED);
		else _currRecord.setStatus(TB_OK);
	}

	bool TableNavigator::moveToFirstRecord() {
		if (!tableValid()) return false;
		_chunkAddr = _t->tableID();
		_chunk_header = _t->table_header();
		_chunk_start_recordID = 0;
		_currRecord.setID(0);
		checkStatus();
		return true;
	}

	bool TableNavigator::haveMovedToNextChunck() {
		if (chunkIsExtended()) {
			_chunk_start_recordID += chunkCapacity();
			_chunkAddr = _chunk_header.nextChunk();
			_t->loadHeader(_chunkAddr, _chunk_header);
			_VR_ByteNo = 0;
			checkStatus();
			return true;
		}
		else return false;
	}

	void TableNavigator::moveToThisRecord(int recordID) {
		getFirstValidRecordByte();
		if (recordID == -1 || recordID == RecordID(-1)) {
			moveToFirstRecord();
			recordID = -1;
			_currRecord.setStatus(TB_BEFORE_BEGIN);
		}
		 else if (recordID >= endStopID()) {
			_currRecord.setStatus(TB_END_STOP);
			recordID = endStopID();
			while (haveMovedToNextChunck());
		}
		else {
			if (recordID < _chunk_start_recordID) moveToFirstRecord();
			while (recordID - _chunk_start_recordID >= chunkCapacity() && haveMovedToNextChunck()) {;}
			_timeValidRecordLastRead = _t->_timeOfLastChange-1;
			_currRecord.setID(recordID);
			checkStatus();
		}
		_currRecord.setID(recordID);
	}

	TB_Size_t  TableNavigator::getAvailabilityByteAddress() const {
		if (_VR_ByteNo == 0) {
			return _chunkAddr + Table::ValidRecordStart;
		}
		else {
			return _chunkAddr + Table::HeaderSize + (_VR_ByteNo - 1) * sizeof(ValidRecord_t);
		}
	}

	uint8_t TableNavigator::getValidRecordIndex() const {
		uint8_t vrIndex = _currRecord.id() - _chunk_start_recordID;
		auto vr_byteNo = validRecordByteNo(vrIndex);
		if (noOfvalidRecordBytes() >= vr_byteNo) {
			_VR_ByteNo = vr_byteNo;
		}
		if (_VR_ByteNo > 0) vrIndex = vrIndex % RDB_B::ValidRecord_t_Capacity;
		return vrIndex;
	}

	TB_Size_t  TableNavigator::getAvailabilityBytesForThisRecord(ValidRecord_t & usedRecords, uint8_t & vrIndex) const {
		// Assumes we are in the correct chunk. Returns the availabilityByteAddr or 0 for byte(0)
		uint8_t old_VR_ByteNo = _VR_ByteNo;
		vrIndex = getValidRecordIndex();
		TB_Size_t availabilityByteAddr = getAvailabilityByteAddress();
		if (old_VR_ByteNo != _VR_ByteNo || _t->outOfDate(_timeValidRecordLastRead)) {
			db()._readByte(availabilityByteAddr, &usedRecords, sizeof(ValidRecord_t)); // invalidated by moving to another ValidRecord_byte or by table-update
			_timeValidRecordLastRead = micros();
			const_cast<TableNavigator *>(this)->_chunk_header.validRecords(usedRecords);
		}
		else {
			usedRecords = _chunk_header.validRecords();
		}
		return availabilityByteAddr;
	}

	bool TableNavigator::isDeletedRecord() {
		RecordID vrIndex;
		ValidRecord_t usedRecord;
		getAvailabilityBytesForThisRecord(usedRecord, vrIndex);
		if (recordIsUsed(usedRecord, vrIndex)) {
			_currRecord.setStatus(TB_OK);
			return false;
		}
		else {
			_currRecord.setStatus(TB_RECORD_UNUSED);
			return true;
		}
	}

	bool TableNavigator::recordIsUsed(ValidRecord_t usedRecords, int vrIndex) const {
		// Bitshifting more than the width is undefined
		if (vrIndex >= RDB_B::ValidRecord_t_Capacity) return false;
		return ((usedRecords & (1 << vrIndex)) > 0);
	};

	DB_Size_t TableNavigator::recordAddress() const {
		return firstRecordInChunk() + (_currRecord.id() - _chunk_start_recordID)  * _t->recordSize();
	}

	//void TableNavigator::loadHeader() {

	//	if (table().dbValid() && _t->outOfDate(_timeValidRecordLastRead)) {
	//		db()._readByte(_chunkAddr, &_chunk_header, Table::HeaderSize);
	//		_timeValidRecordLastRead = micros();
	//		logger() << "Load Chunk Header at " << _chunkAddr << " :" << _chunk_header._asInt_0 << " TblHdr: " << table().table_header()._asInt_0 << L_endl;
	//	}
	//}

	void TableNavigator::saveHeader() {
		db()._writeByte(_chunkAddr, &_chunk_header, Table::HeaderSize);
		_timeValidRecordLastRead = micros();
	}

	ValidRecord_t TableNavigator::getFirstValidRecordByte() const {
		if (_t->outOfDate(_timeValidRecordLastRead)) {
			db()._readByte(_chunkAddr+3, const_cast<ValidRecord_t*>(&_chunk_header._validRecords), 1);
		}
		return _chunk_header.validRecords();
	}

	RecordID TableNavigator::sortedInsert(void * recToInsert
		, bool(*compareRecords)(TableNavigator * left, const void * right, bool lhs_IsGreater)
		, void(*swapRecords)(TableNavigator *original, void * recToInsert)) {
//		logger() << "sortedInsert start" << L_endl;
		
		//auto debugRecord = *reinterpret_cast<const uint32_t*>(recToInsert);
		//logger() << "\nsortedInsert: " << debugRecord << L_endl;
		//if (debugRecord == 587844698) {
		//	bool a = true;
		//}

		moveToThisRecord(_currRecord.signed_id());
		//logger() << "sortedInsert moveTo start Record " << _currRecord.signed_id() << L_endl;
		if (status() == TB_RECORD_UNUSED || status() == TB_BEFORE_BEGIN) ++(*this);
		bool sortOrder = isSmallestFirst(_t->insertionStrategy());
		bool moveToNext = compareRecords(this, recToInsert, sortOrder);
		//logger() << "sortedInsert moveToNext: " << moveToNext << L_endl;
		bool needToMove = true;
		if (status() == TB_END_STOP) { 
			//logger() << "At end!" << L_endl;
			--(*this);
			if (status() == TB_OK) {
				moveToNext = compareRecords(this, recToInsert, sortOrder);
			}
			else { // Empty Table
				//logger() << "Empty Table" << L_endl;
				needToMove = false;
				moveToNext = false;
			}
		}
		int moveDirection = moveToNext ? -1 : 1;
		// Move to insertion point
		int insertionPos = _currRecord.signed_id();
		//logger() << "insertionPos " << insertionPos << L_endl;
		while (needToMove) {
			insertionPos = _currRecord.signed_id();
			(*this) += moveDirection; // to next valid record
			auto thisStatus = status();
			if (thisStatus == TB_BEFORE_BEGIN || status() == TB_END_STOP) break;
			needToMove = (compareRecords(this, recToInsert, sortOrder) == moveToNext);
		//logger() << "sortedInsert needToMove? " << needToMove << L_endl;
		}
		if (moveDirection == -1) {
			insertionPos = _currRecord.signed_id();
		} 

		++insertionPos;

		// Find Space
		moveToThisRecord(insertionPos);
		//logger() << "sortedInsert moveTo insertionPos " << insertionPos << L_endl;
		ValidRecord_t validRecords;
		uint8_t vrIndex;
		getAvailabilityBytesForThisRecord(validRecords, vrIndex);
		auto insertPosTaken = recordIsUsed(validRecords, vrIndex);

		// Shuffle Records
		if (insertPosTaken) {
			auto swapPos = insertionPos;
			if (status() == TB_BEFORE_BEGIN) setStatus(TB_OK); // only if -1
			while (insertPosTaken && status() == TB_OK) {
				swapRecords(this, recToInsert);
				//logger() << "Swapped: " << *reinterpret_cast<const uint32_t*>(recToInsert) << L_endl;
				++swapPos;
				moveToThisRecord(swapPos);
				getAvailabilityBytesForThisRecord(validRecords, vrIndex);
				insertPosTaken = recordIsUsed(validRecords, vrIndex);
			}
		}
		//logger() << "sortedInsert moveTo final insertionPos " << insertionPos << L_endl;
		//logger() << "Rec To Insert: " << *reinterpret_cast<const uint32_t*>(recToInsert) << L_endl;
		return insertionPos;
	}

}
