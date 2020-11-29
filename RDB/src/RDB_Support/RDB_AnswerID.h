#pragma once
#include "RDB_Capacities.h"

namespace RelationalDatabase {
	class RecordSelector;
	/// <summary>
	/// 2Byte Non-Polymorphic Concrete Class used to hold Record ID and Status of a found record
	/// RDB_Answer_Locator.h provides derived classes for retrieving actual records.
	/// </summary>
	class AnswerID {
	public:
		AnswerID() : _id(0), _status(TB_END_STOP) {}
		AnswerID(const RecordSelector & rs);
		AnswerID(RecordID id, TB_Status status) : _id(id), _status(status) {}
		// Queries
		RecordID id() const { return _id; } 
		int signed_id() const { return int8_t(_id); }
		TB_Status status() const { return _status; }
		// Modifiers
		void setID(RecordID id) { _id = id; }
		void setStatus(TB_Status status) { _status = status; }

	protected:
		RecordID _id;
		TB_Status _status;
	};
}