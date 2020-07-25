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

	Dataset_Relay::Dataset_Relay(Query & query, VolatileData * runtimeData)
		: Record_Interface(query, runtimeData, 0)
		, _name("", 6)
		, _status{ 0, ValRange(e_edOneShort, 0, 1) } {}

	I_UI_Wrapper * Dataset_Relay::getField(int fieldID) {
		if (recordID() == -1 || record().status() != TB_OK) return 0;
		//logger() << "Relay recordID(): " << int(recordID()) << " " << record().rec() << L_endl;
		switch (fieldID) {
		case e_name:
			_name = record().rec().name;
			return &_name;
		case e_state:
		{
			HardwareInterfaces::UI_Bitwise_Relay & rl = relay(record().id());
			relayController().readPorts();
			rl.getStateFromContoller();
			_status.val = rl.logicalState();
			return &_status;
		}
		default: return 0;
		}
	}

	bool Dataset_Relay::setNewValue(int fieldID, const I_UI_Wrapper * newValue) {
		switch (fieldID) {
		case e_name:
		{
			const StrWrapper * strWrapper(static_cast<const StrWrapper *>(newValue));
			_name = *strWrapper;
			//auto debug = record();
			//debug.rec();
			strcpy(record().rec().name, _name.str());
			setRecordID(record().update());
			break;
		}
		case e_state:
			HardwareInterfaces::UI_Bitwise_Relay & rl = relay(record().id());
			_status = *newValue;
			rl.set(_status.val);
			relayController().updateRelays();
			break;
		}
		return false;
	}
}