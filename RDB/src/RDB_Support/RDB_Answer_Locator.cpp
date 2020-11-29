#include "RDB_Answer_Locator.h"
#include "RDB_TableNavigator.h"
#include "RDB_Query.h"
#include "RDB_Table.h"

#include "../../../HeatingSystem/src/Client_DataStructures/Data_Zone.h"

namespace RelationalDatabase {

	////////////////////////////////////////////////////////////////////////////
	// *************************** Answer_Locator *************************************//
	////////////////////////////////////////////////////////////////////////////

	Answer_Locator::Answer_Locator(const TableNavigator & tableNav)
		: AnswerID(tableNav.answerID())
		, _tb(&tableNav.table())
	{
		if (_tb) {
			_lastReadTabVer = _tb->tableVersion() -1;
			_recordAddress = tableNav.recordAddress();
			_validRecordByteAddress = tableNav.getVRByteAddress(tableNav.curr_vrByteNo());
			_validRecordIndex = tableNav.currVRindex();
		}
	}

	Answer_Locator::Answer_Locator(const RecordSelector & rs)
		: AnswerID(rs)
		, _tb(rs.status() == TB_INVALID_TABLE ? 0 : &rs.tableNavigator().table())
	{
		if (_tb) {
			auto & tableNav = rs.tableNavigator();
			_lastReadTabVer = _tb->tableVersion() - 1;
			_recordAddress = tableNav.recordAddress();
			_validRecordByteAddress = tableNav.getVRByteAddress(tableNav.curr_vrByteNo());
			_validRecordIndex = tableNav.currVRindex();
			statusOnly();
		}
	}

	Answer_Locator::Answer_Locator(const Answer_Locator & al) 
		: AnswerID(al)
		, _tb(al._tb)
		, _lastReadTabVer(_tb ? _tb->tableVersion() - 1 : -1)
		, _recordAddress(al._recordAddress)
		, _validRecordByteAddress(al._validRecordByteAddress)
		, _validRecordIndex(al._validRecordIndex) 
	{}

	Answer_Locator & Answer_Locator::operator = (const TableNavigator & tableNav) {
		AnswerID::setID(tableNav.id());
		AnswerID::setStatus(tableNav.status());
		if (tableNav.status() != TB_INVALID_TABLE) {
			_lastReadTabVer = _tb->tableVersion() - 1;
			_recordAddress = tableNav.recordAddress();
			_tb = &const_cast<TableNavigator &>(tableNav).table();
			_validRecordByteAddress = tableNav.getVRByteAddress(tableNav.curr_vrByteNo());
			_validRecordIndex = tableNav.currVRindex();
		}
		return *this;
	}

	Answer_Locator & Answer_Locator::operator = (const RecordSelector & rs) {
		AnswerID::setID(rs.id());
		AnswerID::setStatus(rs.status());
		if (rs.status() != TB_INVALID_TABLE) {
			_lastReadTabVer = _tb->tableVersion() - 1;
			auto & tableNav = rs.tableNavigator();
			_recordAddress = tableNav.recordAddress();
			_tb = &const_cast<TableNavigator &>(tableNav).table();
			_validRecordByteAddress = tableNav.getVRByteAddress(tableNav.curr_vrByteNo());
			_validRecordIndex = tableNav.currVRindex();
		}
		return *this;

	}

	void Answer_Locator::rec(void * rec) {
		if (_tb && _tb->outOfDate(lastReadTabVer())) {
			statusOnly();
			if (_status == TB_OK) {
				_tb->db()._readByte(_recordAddress, rec, _tb->recordSize());
				_lastReadTabVer = _tb->tableVersion();
		//logger() << "AL_Refresh: " << long(this) << " RecAddr: " << _recordAddress << " at: " << lastReadTabVer() << " Table: " << _tb->tableVersion() <<  L_endl;
			}
		}
	}

	TB_Status Answer_Locator::status(void * thisRec) {
		if (_status != TB_INVALID_TABLE) rec(thisRec);
		return _status;
	}

	TB_Status Answer_Locator::statusOnly() {
		if (_tb->outOfDate(lastReadTabVer())) {
			if (_id == RecordID(-1)) _status = TB_BEFORE_BEGIN;
			else if (_id < _tb->maxRecordsInTable()) {
				ValidRecord_t usedRecords;
				_tb->db()._readByte(_validRecordByteAddress, &usedRecords, sizeof(ValidRecord_t));
				if (usedRecords & (1 << _validRecordIndex)) {
					_status = TB_OK;
				}
				else if (_status != TB_END_STOP) { _status = TB_RECORD_UNUSED; }
			}
			else _status = TB_END_STOP;
		}
		return _status;
	}

	void Answer_Locator::deleteRecord() {
		if (_tb == 0) return;
		ValidRecord_t usedRecords;
		_tb->db()._readByte(_validRecordByteAddress, &usedRecords, sizeof(ValidRecord_t));
		usedRecords &= ~(1 << _validRecordIndex);
		_tb->db()._writeByte(_validRecordByteAddress, &usedRecords, sizeof(ValidRecord_t));
		_lastReadTabVer = _tb->vrByteIsChanged();
		if (_validRecordByteAddress == _tb->tableID()+ Table::ValidRecordStart) _tb->table_header().validRecords(usedRecords);
		_status = TB_RECORD_UNUSED;
	}

	void Answer_Locator::update(const void * rec) {
		if (statusOnly() == TB_OK) {
			_tb->db()._writeByte(_recordAddress, rec, _tb->recordSize());
			_lastReadTabVer = _tb->dataIsChanged();
		}
	}
}
