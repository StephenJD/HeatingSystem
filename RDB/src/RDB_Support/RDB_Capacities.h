#pragma once
namespace RelationalDatabase {
	typedef unsigned short DB_Size_t;
	typedef DB_Size_t TableID; // Chunk Header address in EEPROM.
	typedef TableID TB_Size_t;
	typedef	TableID Record_Size_t;
	typedef unsigned char ValidRecord_t; // Bits set for unavailable records
	typedef unsigned char RecordID; // Unique and stable Index of record in an entire table. Records are never relocated.
	typedef unsigned char NoOf_Recs_t;

	enum TB_Status : unsigned char {
		TB_OK,
		TB_RECORD_UNUSED,
		TB_END_STOP,
		TB_BEFORE_BEGIN,
		TB_TABLE_NOT_OPENED,
		TB_TABLE_FULL,
		TB_INVALID_TABLE
	};
}

