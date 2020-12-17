#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\..\..\RDB\src\RDB.h"
#include "..\..\..\DateTime\src\Date_Time.h"
#include "DateTimeWrapper.h"

namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              Spell RDB Table
	//***************************************************

	struct R_Spell {
		// Spells can be filtered for a Dwelling via their Program
		// When a new Spell is created, it is offered only those for a particular Dwelling 
		constexpr R_Spell(Date_Time::DateTime date, RecordID programID) : date(date), programID(programID) {}
		R_Spell() = default;
		Date_Time::DateTime date;
		RecordID programID = -1; // Initialisation inhibits default aggregate initialisation.
		RecordID field(int fieldIndex) const { return programID; }
		//bool operator < (R_Spell rhs) const { return date < rhs.date ; }
		//bool operator < (Date_Time::DateTime rhs) const { return date < rhs; }
		//operator Date_Time::DateTime() const { return date; }
		bool operator == (R_Spell rhs) const { return date == rhs.date; }
	};
	inline bool operator < (R_Spell lhs, R_Spell rhs) { return lhs.date < rhs.date ; }
	inline bool operator < (R_Spell lhs, Date_Time::DateTime rhs) { return lhs.date < rhs ; }
	inline bool operator < (Date_Time::DateTime lhs, R_Spell rhs) { return lhs < rhs.date ; }

	inline Logger & operator << (Logger & stream, const R_Spell & spell) {
		return stream << F("Spell Date: ") << spell.date << F(" with ProgID: ") << (int)spell.programID;
	}

	//***************************************************
	//              Dataset_Spell
	//***************************************************

	/// <summary>
	/// Obtains a complete record from the DB, based on a query.
	/// Constructed with a RecordInterface, Query and parent object pointer.
	/// The object may be used for any of the fields returned by the query, via getField(fieldID)
	/// </summary>
	class Dataset_Spell : public Dataset {
	public:
		enum streamable { e_date, e_progID };
		Dataset_Spell(I_Record_Interface& recordInterface, Query& query, Dataset* dwelling);
		bool setNewValue(int fieldID, const I_Data_Formatter* val) override;
		//void insertNewData() override;
	};


	//***************************************************
	//              RecInt_Spell
	//***************************************************

	/// <summary>
	/// DB Interface to all Spell Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>
	class RecInt_Spell : public Record_Interface<R_Spell>
	{
	public:
		enum streamable { e_date, e_progID };
		RecInt_Spell();
		I_Data_Formatter * getField(int fieldID) override;
		int recordFieldVal(int selectFieldID) const override { 
			return answer().rec().field(selectFieldID);
		}
	private:
		DateTimeWrapper _startDate; // size is 32 bits.
	};
}

