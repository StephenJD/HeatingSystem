#pragma once
#include "RDB_AnswerID.h"
#include "RDB_Capacities.h"

namespace RelationalDatabase {
	class Table;
	class TableNavigator;
	class RecordSelector;
	class Query;

	/// <summary>
	/// 12 Bytes Abstract class holding Table-pointer and Record address
	/// Provides interface for updating and deleting records
	/// </summary>
	class Answer_Locator : public AnswerID {
	public:
		using AnswerID::status;
		Answer_Locator() = default;
		Answer_Locator(const TableNavigator & rs);
		Answer_Locator(const RecordSelector & rs);
		Answer_Locator(const Answer_Locator & al);
		void deleteRecord();
		Answer_Locator & operator = (const TableNavigator & rs);
		Answer_Locator & operator = (const RecordSelector & rs);
		void update(const void * rec);
		Answer_Locator * operator->() { return this; } // supports -> dereferencing on RecordSelector, returning an Answer_Locator
		AnswerID operator*() { return *this; }
		Table * table() const {return _tb;}
		Table * table() {return _tb;}
		TableID addr() {return _recordAddress;}
		uint8_t lastReadTabVer() const { return _lastReadVRver; }
	protected:
		// Modifiers
		/// <summary>
		/// Reads the record into the derived-class object
		/// </summary>
		void rec(void * rec);
		TB_Status status(void * rec);
		TB_Status statusOnly();
	private:
		Table * _tb = 0;	// 4B
		TableID _recordAddress = 0; // 2B
		TableID _validRecordByteAddress = 0; // 2B
		mutable uint8_t _lastReadVRver = 0; // 1B initialized to _tb->vrVersion()
		uint8_t _validRecordIndex = 0; // 1B
	};
}
