#pragma once
#include "..\LCD_UI\UI_Collection.h"
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include <RDB.h>

namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              Program RDB Tables
	//***************************************************

	struct R_Program {
		// Each Dwelling has a set of program names
		char name[8];
		RecordID dwellingID; // Initialisation inhibits aggregate initialisation. Owning Dwelling
		RecordID field(int fieldIndex) { return dwellingID; }
		bool operator < (R_Program rhs) const { return false; }
		bool operator == (R_Program rhs) const { return true; }
	};

	inline Logger & operator << (Logger & stream, const R_Program & program) {
		return stream << F("Program: ") << program.name << F(" for DwellingID: ") << (int)program.dwellingID;
	}

	//***************************************************
	//              Dataset_Program
	//***************************************************

	/// <summary>
	/// Obtains a complete record from the DB, based on a query.
	/// Constructed with a RecordInterface, Query and parent object pointer.
	/// The object may be used for any of the fields returned by the query, via getField(fieldID)
	/// </summary>
	class Dataset_Program : public Dataset {
	public:
		Dataset_Program(I_Record_Interface& recordInterface, Query& query, Dataset* dwelling) : Dataset(recordInterface,query,dwelling  ) {};
		bool setNewValue(int fieldID, const I_Data_Formatter* val) override;
	};
	
	//***************************************************
	//              RecInt_Program
	//***************************************************

	/// <summary>
	/// DB Interface to all Program Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>
	class RecInt_Program : public Record_Interface<R_Program> {
	public:
		enum streamable { e_id, e_name, e_dwellingID };
		RecInt_Program();
		I_Data_Formatter * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_Data_Formatter * val) override;

	private:
		StrWrapper _name;
	};
}

