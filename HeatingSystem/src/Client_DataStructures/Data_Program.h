#pragma once
#include "..\LCD_UI\UI_Collection.h"
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include <RDB.h>

namespace client_data_structures {
	using namespace LCD_UI;
	//***************************************************
	//              Program UI Edit
	//***************************************************

	//***************************************************
	//              Program Dynamic Class
	//***************************************************

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
		return stream << "Program: " << program.name << " for DwellingID: " << (int)program.dwellingID;
	}

	//***************************************************
	//              Program LCD_UI
	//***************************************************

	/// <summary>
	/// DB Interface to all Program Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>
	class Dataset_Program : public Record_Interface<R_Program> {
	public:
		enum streamable { e_id, e_name, e_dwellingID };
		Dataset_Program(Query & query, VolatileData * volData, I_Record_Interface * parent);
		//int resetCount() override;
		I_UI_Wrapper * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_UI_Wrapper * val) override;

	private:
		StrWrapper _name;
	};
}

