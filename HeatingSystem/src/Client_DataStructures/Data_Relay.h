#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\LCD_UI\I_Data_Formatter.h"
#include <Relay_Bitwise.h>

namespace HardwareInterfaces {
	//***************************************************
	//              UI_Bitwise_Relay
	//***************************************************

	class UI_Bitwise_Relay : public  LCD_UI::VolatileData, public Relay_B {
	public:
		UI_Bitwise_Relay() : Relay_B(0, 0) {}
		// Queries
		RelationalDatabase::RecordID recordID() const { return _recordID; }
		// Modifiers
		void initialise(int recordID, int port, bool activeState) { _recordID = recordID; Relay_B::initialise(port, activeState); }
		void initialise(int recordID, uint8_t relayRec) { _recordID = recordID; Relay_B::initialise(relayRec); }

	private:
		RelationalDatabase::RecordID _recordID = 0;
	};
}

namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              Relay RDB Tables
	//***************************************************

	struct R_Relay {
		char name[5];
		uint8_t relay_B; // Mapped Relay_B. Initialisation inhibits aggregate initialisation. Includes port and active state
		bool operator < (R_Relay rhs) const { return false; }
		bool operator == (R_Relay rhs) const { return true; }
	};

	inline Logger & operator << (Logger & stream, const R_Relay & r_relay) {
		return stream << F("Relay: ") << r_relay.name << F(" Port: ") << HardwareInterfaces::Relay_B(r_relay.relay_B).port();
	}

	//***************************************************
	//              RecInt_Relay
	//***************************************************

	/// <summary>
	/// DB Interface to all Relay Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// A single object may be used to stream and edit any of the fields via getFieldAt
	/// </summary>
	class RecInt_Relay : public Record_Interface<R_Relay> {
	public:
		enum streamable { e_name, e_state };
		RecInt_Relay(VolatileData * runtimeData);
		VolatileData* runTimeData() override { return _runtimeData; }

		I_Data_Formatter * getField(int _fieldID) override;
		bool setNewValue(int _fieldID, const I_Data_Formatter * val) override;
		HardwareInterfaces::UI_Bitwise_Relay & relay(int index) { return static_cast<HardwareInterfaces::UI_Bitwise_Relay*>(runTimeData())[index]; }
	private:
		VolatileData* _runtimeData;
		StrWrapper _name;
		IntWrapper _status;
	};
}