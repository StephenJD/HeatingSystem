#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\HardwareInterfaces\Relay.h"

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
		uint8_t port; // includes port and active state
		Relay & obj(int objID) { return HardwareInterfaces::relays[objID]; }
		bool operator < (R_Relay rhs) const { return false; }
		bool operator == (R_Relay rhs) const { return true; }
	};

	inline std::ostream & operator << (std::ostream & stream, const R_Relay & relay) {
		return stream << "Relay: " << relay.name << " Port: " << (int)relay.port;
	}


	//***************************************************
	//              Relay LCD_UI
	//***************************************************

	class UI_Relay : public I_Record_Interface
	{
	public:

	};

}