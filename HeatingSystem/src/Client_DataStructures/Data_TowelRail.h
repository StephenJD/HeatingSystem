#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\LCD_UI\I_Data_Formatter.h"
#include "..\..\..\RDB\src\RDB.h"
#include "..\..\..\DateTime\src\Date_Time.h"
#include "..\HardwareInterfaces\TowelRail.h"

namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              Zone RDB Tables
	//***************************************************

	struct R_TowelRail {
		char name[8];
		RecordID callTempSens;
		RecordID callRelay;
		RecordID mixValve;
		uint8_t onTemp;
		uint8_t minutes_on;
		bool operator < (R_TowelRail rhs) const { return false; }
		bool operator == (R_TowelRail rhs) const { return true; }
	};

	inline Logger & operator << (Logger & stream, const R_TowelRail & towelRail) {
		return stream << F("TowelRail: ") << towelRail.name << F(" On for ") << towelRail.minutes_on << F(" minutes");
	}

	//***************************************************
	//              TowelRail DB Interface
	//***************************************************

	/// <summary>
	/// DB Interface to all TowelRail Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
	/// A single object may be used to stream and edit any of the fields via getFieldAt
	/// </summary>
	class Dataset_TowelRail : public Record_Interface<R_TowelRail> {
	public:
		enum streamable { e_name, e_onTemp, e_minutesOn, e_secondsToGo, e_TowelRailID };
		Dataset_TowelRail(Query & query, VolatileData * runtimeData, I_Record_Interface * parent);
		I_Data_Formatter * getField(int _fieldID) override;
		bool setNewValue(int _fieldID, const I_Data_Formatter * val) override;
		HardwareInterfaces::TowelRail & towelRail(int index) { return static_cast<HardwareInterfaces::TowelRail*>(runTimeData())[index]; }
	private:
		StrWrapper _name;
		IntWrapper _onTemp;
		IntWrapper _minutesOn;
		IntWrapper _secondsToGo;
	};
}