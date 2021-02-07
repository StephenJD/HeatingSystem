#include "Data_Relay.h"
#include "..\..\..\Conversions\Conversions.h"
#include "..\LCD_UI\UI_FieldData.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;     
	using namespace HardwareInterfaces;

	//***************************************************
	//              Dataset_Relay
	//***************************************************

	RecInt_Relay::RecInt_Relay(UI_Bitwise_Relay* runtimeData)
		: _runtimeData(runtimeData)
		, _name("", 6)
		, _status{ 0, ValRange(e_edOneShort, 0, 1) } {}

	I_Data_Formatter * RecInt_Relay::getField(int fieldID) {
		if (recordID() == -1 || status() != TB_OK) return 0;
		//logger() << "Relay recordID(): " << int(recordID()) << " " << answer().rec() << L_endl;
		switch (fieldID) {
		case e_name:
			_name = answer().rec().name;
			return &_name;
		case e_state:
		{
			HardwareInterfaces::UI_Bitwise_Relay & rl = runTimeData();
			relayController().readPorts();
			rl.getStateFromContoller();
			_status.val = rl.logicalState();
			return &_status;
		}
		default: return 0;
		}
	}

	bool RecInt_Relay::setNewValue(int fieldID, const I_Data_Formatter * newValue) {
		switch (fieldID) {
		case e_name:
		{
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_name = *strWrapper;
			//auto debug = record();
			//debug.rec();
			strcpy(answer().rec().name, _name.str());
			answer().update();
			break;
		}
		case e_state:
			HardwareInterfaces::UI_Bitwise_Relay & rl = runTimeData();
			_status = *newValue;
			rl.set(_status.val);
			relayController().updateRelays();
			break;
		}
		return false;
	}
}