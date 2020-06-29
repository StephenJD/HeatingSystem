#pragma once

#ifdef ZPSIM
#include <map>
extern std::map<long, std::string> ui_Objects;
#endif


namespace Assembly {
	enum tableIndex {TB_ThermalStore, TB_MixValveContr, TB_Display, TB_Relay, TB_TempSensor, TB_TowelRail, TB_Dwelling, TB_Zone, TB_DwellingZone, TB_Program, TB_Profile, TB_TimeTemp, TB_Spell, TB_NoOfTables };
	enum relayNames { R_Flat, R_FlTR, R_HsTR, R_UpSt, R_MFS, R_Gas, R_DnSt, NO_OF_RELAYS };
	enum tempSensName { T_UR, T_DR, T_FR, T_HTrS, T_ETrS, T_FTrS, T_GasF, T_OS, T_Pdhw, T_DHW, T_UUfh, T_DUfh, T_TkTop, T_TkUs, T_TkDs, T_CWin, T_Sol, T_MfF, NO_OF_TEMP_SENSORS };
	enum mixValveControlerNames { M_DownStrs, M_UpStrs, NO_OF_MIX_VALVES };
	enum remoteDisplayNames { D_Bedroom, D_Flat, D_Hall, NO_OF_REMOTE_DISPLAYS };
	enum towelRailNames { W_Ensuite, W_Family, W_Flat, NO_OF_TOWELRAILS };
	enum zoneNames { Z_UpStairs, Z_DownStairs, Z_DHW, Z_Flat, NO_OF_ZONES };
}
