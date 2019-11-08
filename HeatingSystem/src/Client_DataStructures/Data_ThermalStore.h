#pragma once
#include "..\..\..\RDB\src\RDB.h"

namespace client_data_structures {

	//***************************************************
	//              ThermalStore RDB Table
	//***************************************************

	struct R_ThermalStore {
		RelationalDatabase::RecordID GasRelay;
		RelationalDatabase::RecordID GasTS;
		RelationalDatabase::RecordID OvrHeatTS;
		RelationalDatabase::RecordID MidDhwTS;
		RelationalDatabase::RecordID LowerDhwTS;
		RelationalDatabase::RecordID GroundTS;
		RelationalDatabase::RecordID DHWpreMixTS;
		RelationalDatabase::RecordID DHWFlowTS;
		RelationalDatabase::RecordID OutsideTS;
		uint8_t OvrHeatTemp;
		uint8_t DHWflowRate;
		uint8_t DHWflowTime;
		uint8_t Conductivity;
		uint8_t CylDia;
		uint8_t CylHeight;
		uint8_t TopSensHeight;
		uint8_t MidSensHeight;
		uint8_t LowerSensHeight;
		uint8_t BoilerPower;
		bool operator < (R_ThermalStore rhs) const { return false; }
		bool operator == (R_ThermalStore rhs) const { return true; }
	};

	inline Logger & operator << (Logger & stream, const R_ThermalStore & thermalStore) {
		return stream << "ThermalStore";
	}
}