#pragma once
#include <RDB.h>
#include <Logging.h>

namespace client_data_structures {
	using namespace RelationalDatabase;
	//***************************************************
	//              Zone RDB Tables
	//***************************************************

	struct R_Zone {
		char name[7];
		char abbrev[4];
		RecordID callTempSens;
		RecordID callRelay;
		RecordID mixValve;
		uint8_t maxFlowTemp;
		int8_t offsetT;
		uint8_t autoRatio; // heat-in/heat-out : 20 X (flowT-RoomT)/(RoomT-OS_T) 
		uint8_t autoTimeC; 
		uint8_t autoQuality; // Longest Averaging Period(mins/8). If == 0, use manual.
		int8_t autoDelay;
		bool operator < (R_Zone rhs) const { return false; }
		bool operator == (R_Zone rhs) const { return true; }
	};

	inline arduino_logger::Logger & operator << (arduino_logger::Logger & stream, const R_Zone & zone) {
		return stream << F("Zone: ") << zone.name;
	}

	struct R_DwellingZone {
		RecordID dwellingID;
		RecordID zoneID;
		RecordID field(int fieldIndex) {
			if (fieldIndex == 0) return dwellingID; else return zoneID;
		}
		bool operator < (R_DwellingZone rhs) const { return false; }
		bool operator == (R_DwellingZone rhs) const { return true; }
	};

	inline arduino_logger::Logger & operator << (arduino_logger::Logger & stream, const R_DwellingZone & dwellingZone) {
		return stream << F("DwellingZone DwID: ") << (int)dwellingZone.dwellingID << F(" ZnID: ") << (int)dwellingZone.zoneID;
	}
}