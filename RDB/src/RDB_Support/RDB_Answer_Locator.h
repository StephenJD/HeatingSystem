#pragma once
#include "RDB_AnswerID.h"
#include "RDB_B.h"
#include "RDB_Capacities.h"

namespace RelationalDatabase {
	//class RDB_B;
	//class Table;
	class TableNavigator;
	class RecordSelector;
	class Query;

	/// <summary>
	/// Abstract class holding Table-pointer and Record address
	/// Provides interface for updating and deleting records
	/// </summary>
	class Answer_Locator : public AnswerID {
	public:
		using AnswerID::status;
		Answer_Locator() = default;
		Answer_Locator(TableNavigator & rs);
		Answer_Locator(RecordSelector & rs);
		Answer_Locator(const Answer_Locator & al);
		void deleteRecord();
		Answer_Locator & operator = (const TableNavigator & rs);
		Answer_Locator & operator = (const RecordSelector & rs);
		void update(const void * rec);
		Answer_Locator * operator->() { return this; } // supports -> dereferencing on RecordSelector, returning an Answer_Locator
		AnswerID operator*() { return *this; }
	protected:
		// Modifiers
		/// <summary>
		/// Reads the record into the derived-class object
		/// </summary>
		void rec(void * rec);
		TB_Status status(void * rec);
		Table * _tb = 0;
	private:
		TB_Status statusOnly();
		mutable unsigned long _lastRead = 0;
		TableID _recordAddress = 0;
		TableID _validRecordByteAddress = 0;
		unsigned char _validRecordIndex = 0;
	};

	/// <summary>
	/// Template class holding a copy record
	/// Provides interface accessing fields
	/// </summary>
	template <typename Record_T>
	class Answer_R : public Answer_Locator {
	public:
		Answer_R(RecordSelector & rs) : Answer_Locator(rs) {}
		Answer_R(TableNavigator & tableNaviagator) : Answer_Locator(tableNaviagator) {}
		Answer_R(Answer_Locator & answerLocator) : Answer_Locator(answerLocator) {}
		Answer_R() = default;

		Answer_R & operator =(const RecordSelector_T<Record_T> & rs) {
			Answer_Locator::operator=(rs);
			return *this;
		}

		Answer_R & operator =(const RecordSelector & rs) {
			Answer_Locator::operator=(rs);
			return *this;
		}

		// Queries
		// Modifiers
		RecordID field(int fieldIndex) { return rec().field(fieldIndex); }
		Answer_R * operator->() { return this; }

		TB_Status status() {
			return Answer_Locator::status(&_rec);
		};

		const Record_T & rec() const { return const_cast<Answer_R *>(this)->rec(); }

		Record_T & rec() {
			Answer_Locator::rec(&_rec);
			return _rec;
		}

		Record_T & get() {
			return _rec;
		}

		RecordID update() {
			if (isSorted(_tb->insertionStrategy())) {
				deleteRecord();
				auto tableNav = TableNavigator(_tb);
				auto insertPos = tableNav.insert(&_rec);
				*this = tableNav;
			}
			else {
				Answer_Locator::update(&_rec);
			}
			return id();
		}

	private:
		Record_T _rec;
	};
}
