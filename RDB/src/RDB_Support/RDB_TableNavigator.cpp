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
			_lastReadVRversionNo = t->vrVersion();
			_lastReadHdrVersionNo = t->hdrVersion();
		}
		else {
			//logger() << "Null Table Ptr\n";
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
	DB_Size_t TableNavigator::firstRecordInChunk() const { return _chunkAddr + Table::HeaderSize + (lastVRbyteNo() * sizeof(ValidRecord_t)); }
	bool TableNavigator::tableValid() { return _t ? _t->dbValid() : false; }
	RDB_B & TableNavigator::db() const { return _t->db(); }
	Record_Size_t TableNavigator::recordSize() const { return _t->recordSize(); }
	NoOf_Recs_t TableNavigator::chunkCapacity() const { return _t->maxRecordsInChunk(); }
	NoOf_Recs_t TableNavigator::endStopID() const { return _t->maxRecordsInTable(); }

	// *************** Insert, Update & Delete ***************

	RecordID TableNavigator::insertRecord(const void * newRecord) {
		if (_currRecord.status() == TB_BEFORE_BEGIN) _currRecord.setID(0);
		auto targetRecordID = _currRecord.id();
		auto unusedRecordID = reserveUnusedRecordID();
		auto status_OK = true;
		//logger() << "Insert: " << *reinterpret_cast<const uint32_t*>(newRecord) << L_endl;
		if (isSorted(_t->insertionStrategy()) && unusedRecordID < targetRecordID) {			
			//logger() << F(" shuffle back into: ") << (int)unusedRecordID << L_endl;
			status_OK = shuffleRecordsBack_OK(unusedRecordID, targetRecordID);
			targetRecordID = unusedRecordID;
			unusedRecordID = reserveUnusedRecordID();
			//logger() << F(" unusedRecordID: ") << (int)unusedRecordID << L_endl;

		} else targetRecordID = unusedRecordID;
		_currRecord.setID(unusedRecordID);
		if (_currRecord.id() >= _t->maxRecordsInTable()) {
			if (_t->db().extendTable(*this)) {
				haveMovedToNextChunck();
			}
			else {
				_currRecord.setStatus(TB_TABLE_FULL);
				return _currRecord.id();
			}
		}
		//logger() << "Save Record[" << _currRecord.id() << "] at " << recordAddress() << L_endl;
		if (status_OK) {
			status_OK = _t->db()._writeByte(recordAddress(), newRecord, _t->recordSize());
			_currRecord.setStatus(TB_OK);
			_lastReadVRversionNo = _t->dataIsChanged();
		}
		return targetRecordID;
	}

	bool TableNavigator::shuffleRecordsBack_OK(RecordID start, RecordID end) {
		--end;
		moveToThisRecord(start);
		while (start < end) {
			auto currVRbyteNo = _VR_ByteNo;
			while (start < end && currVRbyteNo == _VR_ByteNo) {
				auto writeAddress = recordAddress();
				++start;
				moveToThisRecord(start); // move to possibly unused record
				auto readAddress = recordAddress();
				if (!db().moveRecords_OK(readAddress, writeAddress, recordSize())) return false;
			}
			shuffleValidRecordsByte(currVRbyteNo, start==end || status() == TB_RECORD_UNUSED);
		}
		return true;
	}

	NoOf_Recs_t TableNavigator::thisVRcapacity() const { // 1-8
		// for small chunks, VRcapacity and chunkCapacity will be less than 8!
		auto vrStartIndex = _VR_ByteNo * RDB_B::ValidRecord_t_Capacity; // 0,8,16
		auto maxVRIndex = chunkCapacity() - vrStartIndex; // within a chunk, startRecID might be 
		if (maxVRIndex < RDB_B::ValidRecord_t_Capacity) return maxVRIndex;
		else return RDB_B::ValidRecord_t_Capacity;
	}

	void TableNavigator::shuffleValidRecordsByte(int vrBytoNo, bool shiftIn_VacantRecord) {
		loadVRByte(vrBytoNo);
		currVR_Byte() >>= 1;
		constexpr ValidRecord_t topBit = 1 << (RDB_B::ValidRecord_t_Capacity - 1);
		currVR_Byte() |= topBit;
		if (shiftIn_VacantRecord) {
			ValidRecord_t maxBit = 1 << (thisVRcapacity()-1);
			currVR_Byte() &= ~maxBit;
		}
		saveVRByte(vrBytoNo);
	}

	// *************** Iterator Functions ***************

	void TableNavigator::moveToLastRecord() {
		moveToThisRecord(endStopID());
		moveToFirstUsedRecord(-1);
	}	
	
	void TableNavigator::next(RecordSelector & recSel, int moveBy) {
		operator+=(moveBy);
	}
	
	TableNavigator & TableNavigator::operator+=(int offset) {
		int direction = (offset < 0 ? -1 : 1);
		_currRecord.setID(_currRecord.signed_id() + offset);
		moveToFirstUsedRecord(direction);
		return *this;
	}

	// ********  First Used Record ***************
	void TableNavigator::moveToFirstUsedRecord(int direction) {
		auto currID = _currRecord.signed_id();
		moveToThisRecord(_currRecord.signed_id());

		if (_currRecord.status() == TB_BEFORE_BEGIN) {
			if (direction > 0) {
				_currRecord.setID(0);
				moveToUsedRecordInThisChunk(direction);
			}
		}
		else {
			if (_currRecord.status() == TB_END_STOP) {
				if (direction < 0 && _currRecord.id() > 0) moveToThisRecord(_currRecord.id() -1);
			} 
			
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
		bool withinChunk = true;
		do vrIndex = loadAndGetVRindex();
		while (!haveMovedToUsedRecord(vrIndex, direction) && (withinChunk = isWithinChunk()));
		return withinChunk;
	}

	bool TableNavigator::haveMovedToUsedRecord(uint8_t initialVRindex, int direction) {
		auto VRcapacity = thisVRcapacity();

		// Lambdas
		auto moveIDtoEndOfVRbyte = [direction, VRcapacity, this]() {
			_currRecord.setID(_currRecord.id() + (direction == -1 ? -VRcapacity : VRcapacity));
		};

		// Algorithm
		if (currVR_Byte() == 0) {
			moveIDtoEndOfVRbyte();
			return false;
		}
		ValidRecord_t recordMask = 1 << initialVRindex;
		if (currVR_Byte() != ValidRecord_t(-1)) {
			while (recordMask && (currVR_Byte() & recordMask) == 0) {
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
		// Look in current ValidRecordByte, then from start of current chunk move _currRecord.id to unused record, or one-past this chunk
		auto lastVRbyte = lastVRbyteNo();
		if (_VR_ByteNo > lastVRbyte) _VR_ByteNo = lastVRbyte;
		RecordID unusedRecID = _chunk_start_recordID + _VR_ByteNo * RDB_B::ValidRecord_t_Capacity;
		_currRecord.setID(unusedRecID);
		uint8_t vrIndex = loadAndGetVRindex();
		bool gotUnusedRecord = haveReservedUnusedRecord(unusedRecID);
		if (!gotUnusedRecord && _VR_ByteNo != 0) {
			unusedRecID = _chunk_start_recordID;
			_currRecord.setID(unusedRecID);
			vrIndex = loadAndGetVRindex();
			gotUnusedRecord = haveReservedUnusedRecord(unusedRecID);
		}
		while (!gotUnusedRecord && _VR_ByteNo <= lastVRbyteNo()) {
			_currRecord.setID(_currRecord.id() + thisVRcapacity()); // to move VR-byte on
			loadAndGetVRindex();
			unusedRecID = _currRecord.id();
			gotUnusedRecord = haveReservedUnusedRecord(unusedRecID);
		}

		if (gotUnusedRecord) {
			saveVRByte(_VR_ByteNo);
		}
		_currRecord.setID(unusedRecID);
		return gotUnusedRecord;
	}

	bool TableNavigator::haveReservedUnusedRecord(RecordID & unusedRecID) const {
		// assumes unusedRecID is ValidRecord_start_ID
		if (currVR_Byte() == ValidRecord_t(-1)) {
			unusedRecID += thisVRcapacity();
			return false;
		}
		else {
			ValidRecord_t recordMask = 1;
			while ((currVR_Byte() & recordMask) > 0) {
				recordMask <<= 1;
				++unusedRecID;
			}
			// Set bit to 1
			currVR_Byte() |= recordMask;
			return true;
		}
	}

	// ********  Last Used Record for end() ***************

	bool TableNavigator::getLastUsedRecordInThisChunk(RecordID & usedRecID) {
		int lastVRbyte = lastVRbyteNo();
		int vrByteNo = 0;
		loadVRByte(vrByteNo);
		RecordID vr_startID = 0;
		bool found = false;
		do {
			found |= lastUsedRecord(currVR_Byte(), usedRecID, vr_startID);
			if (++vrByteNo > lastVRbyte) break;
			loadVRByte(vrByteNo);
		} while (true);
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
		if (_t->tableID() == _chunkAddr) _t->table_header() = _chunk_header;
		saveHeader();
	}

	// ********  Navigation Support ***************

	void TableNavigator::checkStatus() {
		RecordID vrIndex = loadAndGetVRindex();
		if (!recordIsUsed(vrIndex)) _currRecord.setStatus(TB_RECORD_UNUSED);
		else _currRecord.setStatus(TB_OK);
	}

	bool TableNavigator::moveToFirstRecord() {
		if (!tableValid()) return false;
		_chunkAddr = _t->tableID();
		_chunk_header = _t->table_header();
		_chunk_start_recordID = 0;
		_currRecord.setID(0);
		_VR_ByteNo = 0;
		checkStatus();
		return true;
	}

	void TableNavigator::loadChunkHeader() const {
		_t->loadHeader(_chunkAddr, _chunk_header);
		_VR_ByteNo = 0;
		_lastReadVRversionNo = _t->vrVersion() - 1;
		_lastReadHdrVersionNo = _t->hdrVersion();
	}

	bool TableNavigator::haveMovedToNextChunck() {
		if (_t->hdrOutOfDate(_lastReadHdrVersionNo)) {
			loadChunkHeader();
		}
		if (chunkIsExtended()) {
			_chunk_start_recordID += chunkCapacity();
			_chunkAddr = _chunk_header.nextChunk();
			loadChunkHeader();
			setID(_chunk_start_recordID);
			checkStatus();
			return true;
		}
		else return false;
	}

	bool TableNavigator::haveMovedToNextVR(int recordID) {
		auto thisVRbytoNo = validRecordByteNo(recordID - _chunk_start_recordID);
		auto lastByteNo = lastVRbyteNo();
		if (thisVRbytoNo > lastByteNo) {
			//_VR_ByteNo = lastByteNo;
			//_lastReadVRversionNo = _t->vrVersion() - 1;
			return false;
		}
		if (thisVRbytoNo != _VR_ByteNo ) {
			_lastReadVRversionNo = _t->vrVersion() - 1;
			_VR_ByteNo = thisVRbytoNo;
			return true;
		}
		return false;
	}

	void TableNavigator::moveToThisRecord(int recordID) {
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
			if (recordID < _chunk_start_recordID) 
				moveToFirstRecord();
			
			if (!haveMovedToNextVR(recordID)) {
				while (recordID - _chunk_start_recordID >= chunkCapacity() && haveMovedToNextChunck()) { ; }
			}
			_currRecord.setID(recordID);
			checkStatus();
		}
		_currRecord.setID(recordID);
	}

	TB_Size_t  TableNavigator::getVRByteAddress(int vr_byteNo) const {
		return _chunkAddr + Table::ValidRecordStart + (vr_byteNo * sizeof(ValidRecord_t));
	}

	TB_Size_t TableNavigator::currOffsetInChunk() const {
		return _currRecord.id() - _chunk_start_recordID;
	}

	uint8_t TableNavigator::currVRindex() const {
		return currOffsetInChunk() % RDB_B::ValidRecord_t_Capacity;
	}

	uint8_t TableNavigator::loadAndGetVRindex() const {
		// Assumes we are in the correct chunk. Loads VRByte and returns the index within the correct VRByte
		loadVRByte(validRecordByteNo(currOffsetInChunk()));
		return currVRindex();
	}

	bool TableNavigator::isDeletedRecord() {
		RecordID vrIndex = loadAndGetVRindex();	
		if (recordIsUsed(vrIndex)) {
			_currRecord.setStatus(TB_OK);
			return false;
		}
		else {
			_currRecord.setStatus(TB_RECORD_UNUSED);
			return true;
		}
	}

	bool TableNavigator::recordIsUsed(int vrIndex) const {
		// Bitshifting more than the width is undefined
		return ((currVR_Byte() & (1 << vrIndex)) > 0);
	};

	DB_Size_t TableNavigator::recordAddress() const {
		return firstRecordInChunk() + (_currRecord.id() - _chunk_start_recordID)  * _t->recordSize();
	}

	void TableNavigator::saveHeader() {
		db()._writeByte(_chunkAddr, &_chunk_header, Table::HeaderSize);
		_lastReadVRversionNo = _t->tableIsChanged(false);
		_lastReadHdrVersionNo = _t->hdrVersion();
	}

	void TableNavigator::saveVRByte(int vrByteNo) const {
		int vrAddr = getVRByteAddress(vrByteNo);
		db()._writeByte(vrAddr, &currVR_Byte(), sizeof(ValidRecord_t));
		_lastReadVRversionNo = _t->vrByteIsChanged();
		if (_t->tableID() == _chunkAddr && vrByteNo == 0) _t->table_header().validRecords(currVR_Byte());
	}

	void TableNavigator::loadVRByte(int vrByteNo) const {
		int vrAddr = getVRByteAddress(vrByteNo);
		if (_t->vrOutOfDate(_lastReadVRversionNo) || vrByteNo != _VR_ByteNo) {
			if (vrByteNo > lastVRbyteNo()) {
				_lastReadVRversionNo = _t->vrVersion() - 1;
			} else {
				db()._readByte(vrAddr, &_chunk_header._validRecords, sizeof(ValidRecord_t));
				_lastReadVRversionNo = _t->vrVersion();
			}
			_VR_ByteNo = vrByteNo;
		}
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
		//bool needToMove = moveToNext;
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
			//logger() << insertionPos << " sortedInsert needToMove? " << needToMove << L_endl;
		}
		if (moveDirection == -1) {
			insertionPos = _currRecord.signed_id();
		} 

		++insertionPos;

		// Find Space
		moveToThisRecord(insertionPos);
		//logger() << "sortedInsert moveTo insertionPos " << insertionPos << L_endl;
		uint8_t vrIndex = loadAndGetVRindex();	
		auto insertPosTaken = recordIsUsed(vrIndex);

		// Shuffle Records
		if (insertPosTaken) {
			auto swapPos = insertionPos;
			if (status() == TB_BEFORE_BEGIN) setStatus(TB_OK); // only if -1
			while (insertPosTaken && status() == TB_OK) {
				swapRecords(this, recToInsert);
				++swapPos;
				//logger() << "Swapped. Now inserting old rec: " << id() << " at " << swapPos << L_endl;
				moveToThisRecord(swapPos);
				vrIndex = loadAndGetVRindex();
				insertPosTaken = recordIsUsed(vrIndex);
			}
		}
		//logger() << "sortedInsert moveTo final insertionPos " << insertionPos << L_endl;
		//logger() << "Rec To Insert: " << *reinterpret_cast<const uint32_t*>(recToInsert) << L_endl;
		return insertionPos;
	}

}
