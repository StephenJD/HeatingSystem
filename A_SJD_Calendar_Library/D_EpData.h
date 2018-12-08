#if !defined (D_EPDATA_H_)
#define D_EPDATA_H_

///////////////////////////// D_EpData ///////////////////////////////////////////////////////
// Single Responsibility is to derive from EpDataT for each class requiring EEPROM storage //
////////////////////////////////////////////////////////////////////////////////////////////
#include "A__Constants.h"
#include "D_EpDat_A.h"
#include "D_DataStream.h"
#include "D_Operations.h"
#include "D_Edit.h"

////////////////////////////// EpData Template Definition Constants /////////////////////////////////////
//enum templateID {INI_ST, NO_OF_ST, DISPLAY_ST, THM_ST_ST, RELAY_ST, MIXVALVE_ST, TEMP_SNS_ST, OCC_HTR_ST, ZONE_ST, TWLRAD_ST, DATE_TIME_ST, DAYLY_PROG_ST, TIME_TEMP_ST};
//enum noItemsVals	{noDisplays, noTempSensors, noRelays, noMixValves, noOccasionalHts, noZones, noTowelRadCcts, noAllDPs, noAllDTs, noAllTTs,NO_COUNT};
//enum StartAddr {INI_ST,
//				NO_OF_ST		= 6,
//				NO_OF_DTA		= NO_OF_ST + 2, // = 8
//				THM_ST_ST		= NO_OF_DTA + NO_COUNT, // = 18
//				DISPLAY_ST		= THM_ST_ST+2,
//				RELAY_ST		= DISPLAY_ST+2, // = 22
//				MIXVALVE_ST		= RELAY_ST+2,
//				TEMP_SNS_ST		= MIXVALVE_ST+2,
//				OCC_HTR_ST		= TEMP_SNS_ST+2,
//				ZONE_ST			= OCC_HTR_ST+2,
//				TWLRAD_ST		= ZONE_ST+2, // = 32
//				DATE_TIME_ST	= TWLRAD_ST+2,
//				DAYLY_PROG_ST	= DATE_TIME_ST+2, // =36
//				TIME_TEMP_ST	= DAYLY_PROG_ST+2}; // = 38

enum nameLenght {NO_NAME=0, RELAY_N=4, MIXV_N=6, TEMPS_N=4, OCCHTR_N=8, ZONE_N=6, ZONE_A=2, TWLRL_N=6, DLYPRG_N=9};

////////////////////////////// EpData Val Enums /////////////////////////////////////
enum IniSetVals		{INI_COUNT = 9};
enum InfoMenuVals	{SET_UP,I_THRM_STORE,I_ZONES,I_MIX_VALVE,I_TWL_RAD,I_EVENTS,I_TEST, INFO_COUNT};
enum displayVals	{dsMode, dsContrast, dsBrightBacklight, dsDimBacklight, dsAddress, dsZone, dsDimPhoto, dsBrightPhoto, DS_COUNT};
enum thermStoreVals {DHWflowRate, DHWflowTime, GasRelay, GasTS, OvrHeatTemp, OvrHeatTS, MidDhwTS, LowerDhwTS, GroundTS, DHWpreMixTS, DHWFlowTS,
	OutsideTS, Conductivity, CylDia, CylHeight, TopSensHeight, MidSensHeight, LowerSensHeight, BoilerPower, TMS_COUNT};
enum relayVals		{rDeviceAddr, rPortNo, rActiveState, RL_COUNT};
enum relayNames		{r_Flat, r_FlTR, r_HsTR, r_UpSt, r_MFS, r_Gas, r_DnSt};
enum mixValveVals	{mvFlowSensID, mvStoreSensor, MV_COUNT};
enum tempSensName {
    F_R, F_TrS, H_TrS, E_TrS, U_R, D_R, OS, CWin, Pdhw, DHW, U_UF, D_UF, Sol, TkDs, TkUs, TkTop, GasF, MF_F, NO_OF_TS};
enum occHtrVals		{FlowTS, ThrmStrTS, MinTempDiff, MinFlowTemp, OHRelay, OH_COUNT};
enum zoneSetVals	{zCallRelay, zCallTempSensr, zFlowTempSensr, zMaxFlowT, zMixValveID, zWarmBoost, Z_SET_VALS}; // Zone Initial Setup. Stops at zMaxFlowT for DHW zone.
enum zoneUserVals	{zOffset = Z_SET_VALS, zFlags, zManHeat, zAutoFinalTmp, zAutoTimeConst, Z_NUM_USR_VALS};
enum zoneVals		{E_NoOfDTs = Z_NUM_USR_VALS, E_firstDT, E_NoOfDPs, E_firstDP, Z_NUM_VALS}; // Z_COUNT}; // Private records
enum twlRadVals		{tr_CallRelay, tr_CallTempSensr, tr_FlowTempSensr, tr_MaxFlowT, tr_MixValveID, tr_RunTime, TR_COUNT};
enum dpVals			{E_days, E_temp = E_days, E_dpType, E_noOfTTs, E_firstTT, E_period = E_firstTT, DP_COUNT};
enum ttVals			{E_ttTime,E_ttTemp,TT_COUNT, DT_COUNT=4, TS_COUNT = 1, NILL_SETUP = 0};
enum E_dpTypes		{E_dpNew = 0, E_dpRefresh = 1, E_dpWeekly = 64,E_dpDayOff = 128,E_dpInOut = 192,E_dpRefCount = 64};

////////////////////////////// EpData Template Definitions /////////////////////////////////////
//					ID,			noOfVals,	noOfSetup,	nameLength, abbrevLength
#define INI_DEF		INI_ST,		0,			INI_COUNT,	NO_NAME
#define NOOF_DEF	NO_OF_ST,	NO_COUNT,	noAllDPs,	NO_NAME
#define DISPL_DEF	DISPLAY_ST,	DS_COUNT,	dsDimPhoto,	NO_NAME
#define THRMST_DEF	THM_ST_ST,	TMS_COUNT,	TMS_COUNT,	NO_NAME
#define RELAY_DEF	RELAY_ST,	RL_COUNT,	RL_COUNT,	RELAY_N
#define MIXV_DEF	MIXVALVE_ST,MV_COUNT,	MV_COUNT,	MIXV_N
#define TEMPS_DEF	TEMP_SNS_ST,TS_COUNT,	NILL_SETUP,	TEMPS_N
#define OCCHTR_DEF	OCC_HTR_ST,	OH_COUNT,	OH_COUNT,	OCCHTR_N
#define ZONE_DEF	ZONE_ST,	Z_NUM_VALS,	Z_SET_VALS,	ZONE_N,		ZONE_A
#define TWLRL_DEF	TWLRAD_ST,	TR_COUNT,	TR_COUNT,	TWLRL_N
#define DTME_DEF	DATE_TIME_ST,DT_COUNT,	NILL_SETUP,	NO_NAME
#define DLYPRG_DEF	DAYLY_PROG_ST,DP_COUNT,	NILL_SETUP,	DLYPRG_N
#define TMTP_DEF	TIME_TEMP_ST,TT_COUNT,	NILL_SETUP,	NO_NAME
#define INFO_DEF	INFO_ST,	0,			INFO_COUNT,	NO_NAME
#define EVENT_DEF	EVENT_ST,	0,			NILL_SETUP,	NO_NAME



////////////////////////////// NoOf_EpD /////////////////////////////////////

class NoOf_EpD: public EpDataT<NOOF_DEF> {
public:
	NoOf_EpD(){}
	//void setDataStart(U2_epAdrr start) {dataStart = NO_OF_ST;}
	//const U2_epAdrr getDataStart() const {return NO_OF_ST;}
	//void setVal(U1_ind vIndex, U1_byte val);
	//U1_byte getVal(U1_ind vIndex) const;	
	U1_byte operator() (noItemsVals myItem);
	void loadDefaultStartAddr();
	void incrementNoOf(noItemsVals myItem, S1_byte amount);
private: 
};

////////////////////////////// Display_EpD /////////////////////////////////////

class Display_EpD : public EpDataT<DISPL_DEF>
{
public:
	Display_EpD(){}
	char * getName() const;

};

/////////////////////////////// Zone_EpD /////////////////////////////////////

class Zone_EpD : public EpDataT<ZONE_DEF>
{
public:
	Zone_EpD(){}
	// extra methods
	//EEProm Data retrieval
	char * getAbbrev() const;
	const U1_byte getNoOfItems() const;
	U1_byte getAutoMode() const;
	U1_byte getGroup() const;
	U1_byte getWeatherSetback() const;
	U2_byte getAutoConstant() const;
	//Data setting
	void setAutoMode(U1_byte mode);
	void setGroup(U1_byte group);
	void setWeatherSetback(U1_byte wSetBack);
	void setAbbrev(const char * abbrev);
	void setAutoConstant(U2_byte);

private:
	friend class NoOf_EpD;
	// ************* Accessor methods for statics needing defining for each sub class 
	const U1_byte getAbbrevSize() const {return ZONE_A;}
	void loadNewValues();
	// extra methods
	bool isHHWzone() const;
};

#endif