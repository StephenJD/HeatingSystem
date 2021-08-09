#pragma once
#include "Rec_Zone.h"
#include "..\HardwareInterfaces\Zone.h"
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\LCD_UI\I_Data_Formatter.h"

#include <RDB.h>

namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              RecInt_Zone
	//***************************************************

	/// <summary>
	/// DB Interface to all Zone Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// A single object may be used to stream and edit any of the fields via getFieldAt
	/// </summary>
	class RecInt_Zone : public Record_Interface<R_Zone> {
	public:
		enum streamable { e_name, e_abbrev, e_reqTemp, e_remoteReqTemp, e_offset, e_isTemp, e_isHeating, e_ratio, e_timeConst, e_quality, e_delay	};
		RecInt_Zone(HardwareInterfaces::Zone* runtimeData);
		HardwareInterfaces::Zone& runTimeData() override { return _runTimeData[recordID()]; }
		I_Data_Formatter * getField(int _fieldID) override;
		bool setNewValue(int _fieldID, const I_Data_Formatter * val) override;
		bool actionOn_LR(int _fieldID, int moveBy) override;

	private:
		HardwareInterfaces::Zone* _runTimeData;
		StrWrapper _name;
		StrWrapper _abbrev;
		IntWrapper _requestTemp;
		IntWrapper _tempOffset;
		IntWrapper _isTemp;
		StrWrapper _isHeating;
		IntWrapper _autoRatio;
		IntWrapper _autoTimeC;
		IntWrapper _autoQuality;
		IntWrapper _autoDelay;
	};
}