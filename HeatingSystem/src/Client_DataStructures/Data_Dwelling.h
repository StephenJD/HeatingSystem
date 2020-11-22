#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\..\..\RDB\src\\RDB.h"

namespace client_data_structures {
	using namespace LCD_UI;
	//***************************************************
	//              Dwelling RDB Tables
	//***************************************************

	struct R_Dwelling {
		char name[8];
		bool operator < (R_Dwelling rhs) const { return false; }
		bool operator == (R_Dwelling rhs) const { return true; }
	};

	inline Logger & operator << (Logger & stream, const R_Dwelling & dwelling) {
		return stream << F("Dwelling: ") << dwelling.name;
	}

	//***************************************************
	//              RecInt_Dwelling
	//***************************************************

	/// <summary>
	/// DB Interface to all Dwelling Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>	
	class RecInt_Dwelling : public Record_Interface<R_Dwelling> {
	public:
		enum streamable { e_name };
		enum subAssembly { s_zone, s_program };
		RecInt_Dwelling();
		I_Data_Formatter * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_Data_Formatter * val) override;

	private:
		StrWrapper _name;
	};
}
