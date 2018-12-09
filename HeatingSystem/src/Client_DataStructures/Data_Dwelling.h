#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include <RDB.h>

#include <ostream>

namespace client_data_structures {
	using namespace LCD_UI;
	//***************************************************
	//              Dwelling UI Edit
	//***************************************************

	//***************************************************
	//              Dwelling Dynamic Class
	//***************************************************

	//***************************************************
	//              Dwelling RDB Tables
	//***************************************************

	struct R_Dwelling {
		char name[8];
		bool operator < (R_Dwelling rhs) const { return false; }
		bool operator == (R_Dwelling rhs) const { return true; }
	};

	inline std::ostream & operator << (std::ostream & stream, const R_Dwelling & dwelling) {
		return stream << "Dwelling: " << dwelling.name;
	}

	//***************************************************
	//              Dwelling LCD_UI
	//***************************************************

	/// <summary>
	/// DB Interface to all Dwelling Data
	/// Provides streamable fields which may be poulated by a database or the runtime-data.
	/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>	
	class Dataset_Dwelling : public Record_Interface<R_Dwelling> {
	public:
		enum streamable { e_name };
		enum subAssembly { s_zone, s_program };
		Dataset_Dwelling(Query & query, VolatileData * volData, I_Record_Interface * parent);
		//I_UI_Wrapper * getFieldAt(int fieldID, int elementIndex ) override;
		I_UI_Wrapper * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_UI_Wrapper * val) override;

	private:
		StrWrapper _name;
	};
}
