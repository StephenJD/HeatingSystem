#pragma once

namespace Assembly {
	constexpr int CLOCK_ADDR = 0;
	constexpr int RDB_START_ADDR = 6;
	enum tableIndex { TB_Relay, TB_TempSensor, TB_Dwelling, TB_Zone, TB_DwellingZone, TB_Program, TB_Profile, TB_TimeTemp, TB_Spell, TB_NoOfTables };
	enum relayNames { R_Flat, R_FlTR, R_HsTR, R_UpSt, R_MFS, R_Gas, R_DnSt, NO_OF_RELAYS };
	enum tempSensName { T_FR, T_FTrS, T_HTrS, T_ETrS, T_UR, T_DR, T_OS, T_CWin, T_Pdhw, T_DHW, T_UUfh, T_DUfh, T_Sol, T_TkDs, T_TkUs, T_TkTop, T_GasF, T_MfF, NO_OF_TEMP_SENSORS };
	enum remoteDisplayNames { D_Bedroom, D_Flat, D_Hall, NO_OF_REMOTE_DISPLAYS };
}
