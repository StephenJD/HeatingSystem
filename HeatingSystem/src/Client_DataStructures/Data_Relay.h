#pragma once
#include "..\HardwareInterfaces\Relay.h"
#include "..\LCD_UI\I_Record_Interface.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;
	//***************************************************
	//              Relay UI Edit
	//***************************************************


	//***************************************************
	//              Relay RDB Tables
	//***************************************************

	struct R_Relay {
		char name[5];
		uint8_t port; // Initialisation inhibits aggregate initialisation. Includes port and active state
		bool operator < (R_Relay rhs) const { return false; }
		bool operator == (R_Relay rhs) const { return true; }
	};

	inline Logger & operator << (Logger & stream, const R_Relay & relay) {
		return stream << "Relay: " << relay.name << " Port: " << relay.port;
	}

	//***************************************************
	//              Relay LCD_UI
	//***************************************************

	class UI_Relay : public I_Record_Interface
	{
	public:

	};

}