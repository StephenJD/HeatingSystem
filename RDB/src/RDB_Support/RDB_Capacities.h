#pragma once
#include "Arduino.h"
namespace RelationalDatabase {
	typedef uint16_t DB_Size_t;
	typedef DB_Size_t TableID; // Chunk Header address in EEPROM.
	typedef TableID TB_Size_t;
	typedef	TableID Record_Size_t;
	typedef uint8_t ValidRecord_t; // Bits set for unavailable records
	typedef uint8_t RecordID; // Unique and stable Index of record in an entire table. Records are never relocated.
	typedef uint8_t NoOf_Recs_t;
	constexpr uint8_t SIZE_OF_PASSWORD = 1;
	
	enum TB_Status : uint8_t {
		TB_OK,
		TB_RECORD_UNUSED,
		TB_END_STOP,
		TB_BEFORE_BEGIN,
		TB_TABLE_NOT_OPENED,
		TB_TABLE_FULL,
		TB_INVALID_TABLE
	};

	enum InsertionStrategy : uint8_t { i_retainOrder, i_reverseOrder, i_09_orderedInsert, i_09_reverseInsert, i_09_randomInsert, i_90_randomInsert, i_90_orderedInsert, i_90_reverseInsert };
}

