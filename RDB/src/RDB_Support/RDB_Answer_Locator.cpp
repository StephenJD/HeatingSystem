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
			_lastRead = _tb->lastModifiedTime() - TIMER_INCREMENT;
			_recordAddress = tableNav.recordAddress();
			_validRecordByteAddress = tableNav.getAvailabilityByteAddress();
			_validRecordIndex = tableNav.getValidRecordIndex();
		}
	}

	Answer_Locator::Answer_Locator(const RecordSelector & rs)
		: AnswerID(rs)
		, _tb(rs.status() == TB_INVALID_TABLE ? 0 : &rs.tableNavigator().table())
	{
		if (_tb) {
			_lastRead = _tb->lastModifiedTime() - TIMER_INCREMENT;
			_recordAddress = rs.tableNavigator().recordAddress();
			_validRecordByteAddress = rs.tableNavigator().getAvailabilityByteAddress();
			_validRecordIndex = rs.tableNavigator().getValidRecordIndex();
			statusOnly();
		}
	}

	Answer_Locator::Answer_Locator(const Answer_Locator & al) 
		: AnswerID(al)
		, _tb(al._tb)
		, _lastRead(_tb ? _tb->lastModifiedTime() - TIMER_INCREMENT : 0)
		, _recordAddress(al._recordAddress)
		, _validRecordByteAddress(al._validRecordByteAddress)
		, _validRecordIndex(al._validRecordIndex) 
	{}

	Answer_Locator & Answer_Locator::operator = (const TableNavigator & tableNav) {
		AnswerID::setID(tableNav.id());
		AnswerID::setStatus(tableNav.status());
		if (tableNav.status() != TB_INVALID_TABLE) {
			_lastRead = _tb->lastModifiedTime() - TIMER_INCREMENT;
			_recordAddress = tableNav.recordAddress();
			_tb = &const_cast<TableNavigator &>(tableNav).table();
			_validRecordByteAddress = tableNav.getAvailabilityByteAddress();
			_validRecordIndex = tableNav.getValidRecordIndex();
		}
		return *this;
	}

	Answer_Locator & Answer_Locator::operator = (const RecordSelector & rs) {
		AnswerID::setID(rs.id());
		AnswerID::setStatus(rs.status());
		if (rs.status() != TB_INVALID_TABLE) {
			_lastRead = _tb->lastModifiedTime() - TIMER_INCREMENT;
			_recordAddress = rs.tableNavigator().recordAddress();
			_tb = &const_cast<TableNavigator &>(rs.tableNavigator()).table();
			_validRecordByteAddress = rs.tableNavigator().getAvailabilityByteAddress();
			_validRecordIndex = rs.tableNavigator().getValidRecordIndex();
		}
		return *this;

	}

	void Answer_Locator::rec(void * rec) {
		if (_tb && _tb->outOfDate(_lastRead)) {
			statusOnly();
			if (_status == TB_OK) {
				_tb->db()._readByte(_recordAddress, rec, _tb->recordSize());
				_lastRead = micros();
		//logger() << "AL_Refresh: " << long(this) << " RecAddr: " << _recordAddress << " at: " << _lastRead << " Table: " << _tb->lastModifiedTime() <<  L_endl;
			}
		}
	}

	TB_Status Answer_Locator::status(void * thisRec) {
		if (_status != TB_INVALID_TABLE) rec(thisRec);
		return _status;
	}

	TB_Status Answer_Locator::statusOnly() {
		if (_tb->outOfDate(_lastRead)) {
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
		_lastRead = micros();
		_tb->tableIsChanged(true);
		_status = TB_RECORD_UNUSED;
	}

	void Answer_Locator::update(const void * rec) {
		if (statusOnly() == TB_OK) {
			_tb->db()._writeByte(_recordAddress, rec, _tb->recordSize());
			_lastRead = micros();
			_tb->tableIsChanged(false);
		}
	}
}
