#include "RDB_AnswerID.h"
#include "RDB_RecordSelector.h"

namespace RelationalDatabase {
	AnswerID::AnswerID(const RecordSelector & rs) : _id(rs.id()), _status(rs.status()) {}
}
