#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\..\..\RDB\src\RDB.h"
#include "..\..\..\DateTime\src\Date_Time.h"
#include "DateTimeWrapper.h"

#ifdef ZPSIM
	#include <ostream>
#endif

namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              Spell RDB Table
	//***************************************************

	struct R_Spell {
		// Spells can be filtered for a Dwelling via their Program
		// When a new Spell is created, it is offered only those for a particular Dwelling 
		Date_Time::DateTime date;
		RecordID programID;
		RecordID field(int fieldIndex) const { return programID; }
		bool operator < (R_Spell rhs) const { return date < rhs.date ; }
		bool operator == (R_Spell rhs) const { return date == rhs.date; }
	};

#ifdef ZPSIM
	inline std::ostream & operator << (std::ostream & stream, const R_Spell & spell) {
		return stream << "Spell Date: " << std::dec << spell.date.day() << "/" << spell.date.getMonthStr() << "/" << spell.date.year() << " with ProgID: " << (int)spell.programID;
	}
#endif

	//***************************************************
	//              Spell DB Interface
	//***************************************************

	/// <summary>
	/// DB Interface to all Spell Data
	/// Provides streamable fields which may be poulated by a database or the runtime-data.
	/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>
	class Dataset_Spell : public Record_Interface<R_Spell>
	{
	public:
		enum streamable { e_date, e_progID };
		Dataset_Spell(Query & query, VolatileData * volData, I_Record_Interface * parent);
		I_UI_Wrapper * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_UI_Wrapper * val) override;
		void insertNewData() override;
		int recordField(int selectFieldID) const override { 
			return record().rec().field(selectFieldID);
		}
	private:
		DateTimeWrapper _startDate; // size is 32 bits.
	};
}

