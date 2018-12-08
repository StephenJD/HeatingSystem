#include "debgSwitch.h"
#include "D_EpData.h"
#include "D_EpManager.h"
#include "DateTime_Run.h"
#include "DateTime_Stream.h"
#include "Display_Run.h"
#include "ThrmStore_Run.h"
#include "ThrmStore_Stream.h"
#include "Relay_Run.h"
#include "MixValve_Run.h"
#include "MixValve_Stream.h"
#include "TempSens_Run.h"
#include "TempSens_Stream.h"
#include "OccasHtr_Run.h"
#include "Zone_Run.h"
#include "Zone_Stream.h"
#include "TowelR_Operations.h"
#include "DailyProg_Run.h"
#include "DailyProg_Stream.h"
#include "TimeTemp_Stream.h"
#include "IniSet_NoOf_Stream.h"
#include "EEPROM_Utilities.h"
#include "Event_Stream.h"
#include "Display_Stream.h"

using namespace EEPROM_Utilities;

#if defined (ZPSIM)
	#include  <iostream>
	using namespace std;
#endif

enum {DisplayCnt = 4, TempSensorCnt = NO_OF_TS, RelayCnt = 7, MixValveCnt = 2, OccasionalHtCnt = 1, ZoneCnt = 4, TowelRadCnt = 3, AllDPCnt = 20, AllDTCnt = 9, AllTTCnt = 22};


template<>
void OperationsT <RunT <EpDataT<INI_DEF> >,IniSet_Stream>::loadDefaults(){}

template<>
void OperationsT <RunT <EpDataT<INFO_DEF> >,InfoMenu_Stream>::loadDefaults(){}

template<>
void OperationsT <RunT <EpDataT<EVENT_DEF> >,Events_Stream>::loadDefaults(){}

template <>
void OperationsT <RunT <NoOf_EpD>,NoOf_Stream>::loadDefaults(){
	logToSD("NoOf::loadDefaults()");
	//enum noItemsVals {noDisplays, noTempSensors, noRelays, noMixValves, noOccasionalHts, noZones, noTowelRadCcts, noAllDPs, noAllDTs, noAllTTs,NO_COUNT};
	setShort(NO_OF_ST,NO_OF_DTA);
	//int debug = noDisplays;
//int debug = epD().getDataStart();
	setVal(noDisplays, DisplayCnt);
//debug = epD().getDataStart();
	setVal(noTempSensors, TempSensorCnt);
	setVal(noRelays,RelayCnt);
	setVal(noMixValves,MixValveCnt);
	setVal(noOccasionalHts,OccasionalHtCnt);
	setVal(noZones,ZoneCnt);
	setVal(noTowelRadCcts,TowelRadCnt);
	setVal(noAllDTs,AllDTCnt);
	setVal(noAllDPs,AllDPCnt);
	setVal(noAllTTs,AllTTCnt);
//debugEEprom(6,10);
	static_cast<NoOf_EpD &>(epD()).loadDefaultStartAddr();

//	lcd.print(" 3DefNos");
}

template<>
void OperationsT <Display_Run, Display_Stream >::loadDefaults(){
	const U1_byte zone[DisplayCnt] = {U1_byte(-1),1,2,0};
	U1_byte ep_index = epD().index();
	logToSD("Display::loadDefaults()",ep_index);

	setVal(dsMode,e_onChange_read_OUTafter);
	setVal(dsContrast,0);

	setVal(dsDimBacklight,13);
	setVal(dsBrightBacklight,30);
	setVal(dsDimPhoto,40);
	setVal(dsBrightPhoto,20);
 
	setVal(dsAddress,ep_index);
	setVal(dsZone,zone[ep_index]);
}

template<>
void OperationsT <ThrmSt_Run, ThrmStore_Stream>::loadDefaults(){
	logToSD("ThrmSt::loadDefaults()");
	setVal(DHWflowRate,21);
	setVal(DHWflowTime,75);
	setVal(GasRelay,r_Gas);
	setVal(GasTS,GasF);
	setVal(OvrHeatTemp,95);
	setVal(OvrHeatTS,TkTop);
	setVal(MidDhwTS,TkUs); // US Flow
	setVal(LowerDhwTS,TkDs); // DS Flow
	setVal(GroundTS,CWin); // Downstairs flow
	setVal(DHWpreMixTS,Pdhw);
	setVal(DHWFlowTS,DHW);
	setVal(OutsideTS,OS);
	setVal(Conductivity,25);
	setVal(CylDia,60);
	setVal(CylHeight,180);
	setVal(TopSensHeight,169);
	setVal(MidSensHeight,138);
	setVal(LowerSensHeight,97);
	setVal(BoilerPower,25);
}

template<>
void OperationsT <Relay_Run, DataStreamT<EpDataT<RELAY_DEF>, Relay_Run, Relay_EpMan, RL_COUNT> >::loadDefaults(){
	//										r_Flat, r_FlTR, r_HsTR, r_UpSt, r_MFS, r_Gas, r_DnSt
	const char name[RelayCnt][RELAY_N+1] =	{"Flat","FlTR",	"HsTR", "UpSt","MFSt", "Gas", "DnSt"};
	const U1_byte port[RelayCnt] =			{ 6,	 1,		 0,		 5,	    2,	    3,	   4};
	U1_byte ep_index = epD().index();
	logToSD("Relay::loadDefaults()",ep_index);

	epD().setName(name[ep_index]);
	setVal(rDeviceAddr,IO8_PORT_OptCoupl);
	setVal(rPortNo,port[ep_index]);
	setVal(rActiveState,IO8_PORT_OptCouplActive);
}

template<>
void OperationsT <MixValve_Run, MixValve_Stream>::loadDefaults(){
	const char name[MixValveCnt][MIXV_N+1] = {"DnS","UpS"};
	const U1_byte flowTS_ID[MixValveCnt] =	 {D_UF,	U_UF};
	const U1_byte sensor[MixValveCnt] =		 {TkDs,TkUs};	
	U1_byte ep_index = epD().index();
	logToSD("MixValve::loadDefaults()",ep_index);

	epD().setName(name[ep_index]);
	setVal(mvFlowSensID,flowTS_ID[ep_index]);
	setVal(mvStoreSensor,sensor[ep_index]);
}

template<>
void OperationsT <TempSens_Run,TempSens_Stream>::loadDefaults(){
	// index									0		1		2		3		4		5		6		7		8		9		10		11		12		13		14		15		16		17
	//enum tempSensName {						F_R,	F_TrS,	H_TrS,	E_TrS,	U_R,	D_R,	OS,		CWin,	Pdhw,	DHW,	U_UF,	D_UF,	Sol,	TkDs,	TkUs,	TkTop,	GasF,	MF_F
	const char name[TempSensorCnt][TEMPS_N+1] = {"Flat","FlTR",	"HsTR",	"EnST",	"UpSt",	"DnSt",	"OutS",	"Grnd",	"Pdhw",	"DHot",	"US-F",	"DS-F",	"TkMf",	"TkDs",	"TkUs",	"TkTp",	"GasF",	"MFBF"};
	const U1_byte addr[TempSensorCnt] =			{ 0x70,	 0x72,	0x71,	0x76,	0x36,	0x74,	0x2B,	0x35,	0x37,	0x28,	0x2C,	0x4F,	0x75,	0x77,	0x2E,	0x2D,	0x4B,	0x2F};
	U1_byte ep_index = epD().index();
	logToSD("TempSens::loadDefaults()",ep_index);

	epD().setName(name[ep_index]);
	setVal(0,addr[ep_index]);
}

template<>
void OperationsT <OccasHtr_Run, DataStreamT<EpDataT<OCCHTR_DEF>, OccasHtr_Run, OccasHtr_EpMan, OH_COUNT> >::loadDefaults(){
	epD().setName("Stove0");
	setVal(FlowTS,MF_F);
	setVal(ThrmStrTS,Sol);
	setVal(MinTempDiff,2);
	setVal(MinFlowTemp,45);
	setVal(OHRelay,r_MFS);
}

template<>
void OperationsT <Zone_Run, Zone_Stream>::loadDefaults(){
	const char name[ZoneCnt][ZONE_N+1] =		{"Down S","Up-Str",	"Flat  ","DHW   "};
	const char abbrev[ZoneCnt][ZONE_A+1] =		{"DS",	  "US",		"HL",	"HW"};
	const U1_byte noDPs[ZoneCnt] =					{6,			3,		3,		8};
	const U1_byte noDTs[ZoneCnt] =					{6,			1,		1,		1};
	const U1_byte firstDP[ZoneCnt] =				{0,			6,		9,		12};
	const U1_byte firstDT[ZoneCnt] =				{0,			6,		7,		8};
	const U1_byte callTempSens[ZoneCnt] =			{D_R,		U_R,	F_R,	Pdhw};
	const U1_byte flowTempSens[ZoneCnt] =			{D_UF,		U_UF,	U_UF,	GasF};
	const U1_byte callRelay[ZoneCnt] =				{r_DnSt,	r_UpSt,	r_Flat,	r_Gas};
	const U1_byte mixValveID[ZoneCnt] =				{0,			1,		1,		U1_byte(-1)};
	const U1_byte maxFlowT[ZoneCnt] =				{45,		55,		55,		80};
	const U1_byte offset[ZoneCnt] =					{U1_byte(-1),		0,		0,		0};
	const U1_byte autoFinalTmp[ZoneCnt] =			{21,		20,		22,		56};
	const U2_byte autoTimeConst[ZoneCnt] =			{522,		139,	698,	29};
	const U1_byte autoMode[ZoneCnt] =				{1,			1,		1,		1};
	const U1_byte manHeat[ZoneCnt] =				{120,		54,		55,		1};
	U1_byte ep_index = epD().index();
	logToSD("Zone::loadDefaults()",ep_index);

	static_cast<Zone_EpD &>(epD()).setName(name[ep_index]);
	static_cast<Zone_EpD &>(epD()).setAbbrev(abbrev[ep_index]);

	setVal(E_NoOfDPs,noDPs[ep_index]);
	setVal(E_NoOfDTs,noDTs[ep_index]);
	setVal(E_firstDP,firstDP[ep_index]);
	setVal(E_firstDT,firstDT[ep_index]);
	setVal(zCallTempSensr,callTempSens[ep_index]);
	setVal(zFlowTempSensr,flowTempSens[ep_index]);
	setVal(zCallRelay,callRelay[ep_index]);
	setVal(zMixValveID,mixValveID[ep_index]);
	setVal(zMaxFlowT,maxFlowT[ep_index]);
	setVal(zOffset,offset[ep_index]);
	setVal(zWarmBoost,20); // Flow temp above equilibrium to heat room quickly 
	setVal(zAutoFinalTmp,autoFinalTmp[ep_index]); 
	static_cast<Zone_EpD &>(epD()).setAutoConstant(autoTimeConst[ep_index]);
	setVal(zManHeat, manHeat[ep_index]);
	static_cast<Zone_EpD &>(epD()).setAutoMode(autoMode[ep_index]);
}

void Zone_EpD::loadNewValues(){
	setVal(E_NoOfDPs,0);
	setVal(E_NoOfDTs,1); // 0 signifies Towel Rail
	// We must set E_firstDT and E_firstDP to last valid DT/DP if noOfDT/DP == 0
	U1_byte nextDT = 0;
	U1_byte nextDP = 0;
	if (ep_index > 0) {
		//nextDT = zones[ep_index-1].getVal(E_firstDT) + zones[ep_index-1].getVal(E_NoOfDTs);
		//nextDP = zones[ep_index-1].getVal(E_firstDP) + zones[ep_index-1].getVal(E_NoOfDPs);
	} else {nextDT = 0; nextDP = 0;}
	setVal(E_firstDT,nextDT);
	setVal(E_firstDP,nextDP);

	//setVal(currentDP,0);
	setVal(zCallRelay,0);
	setVal(zFlowTempSensr,1);
	setVal(zMaxFlowT,35);
	setVal(zCallTempSensr,1);
	setVal(zMixValveID,0);
	setVal(zWarmBoost,20); // Flow temp above equilibrium to heat room quickly 
	//setVal(zWeatherSB, 0);
	setVal(zOffset, 0);

	//setVal(autoMode,1);
	//setVal(autoProx, 5); // 5/10ths degree at start/end of warm period
	setVal(zAutoFinalTmp,60); // 1hr before warm period
	setVal(zAutoTimeConst, 60);
	setVal(zManHeat ,225); // mins per 0.5 degree heating
	char fieldText[ZONE_N+1];
	strcpy(fieldText,"Zone");
	//strcat(fieldText,cIntToStr(ep_index+1,1));
	setName(fieldText);
	strcpy(fieldText,"Z");
	//strcat(fieldText,cIntToStr(ep_index+1,1));
	setAbbrev(fieldText);
}

void TowelR_Operations::loadDefaults(){
	const char name[TowelRadCnt][TWLRL_N+1] =	{"Fam-TR","Ens-TR",	"Flt-TR"};
	const U1_byte callTempSens[TowelRadCnt] =		{H_TrS,	E_TrS,	F_TrS};
	const U1_byte flowTempSens[TowelRadCnt] =		{U_UF,	U_UF,	U_UF};
	const U1_byte callRelay[TowelRadCnt] =			{r_HsTR,r_HsTR,	r_FlTR};
	const U1_byte mixValveID[TowelRadCnt] =			{1,			1,		1};
	U1_byte ep_index = epD().index();

	epD().setName(name[ep_index]);
	setVal(zCallTempSensr,callTempSens[ep_index]);
	setVal(zFlowTempSensr,flowTempSens[ep_index]);
	setVal(zCallRelay,callRelay[ep_index]);
	setVal(zMixValveID,mixValveID[ep_index]);
	setVal(zMaxFlowT,55);
	setVal(tr_MaxFlowT,TOWEL_RAIL_FLOW_TEMP);
	setVal(tr_RunTime,60);
}

template<>
void OperationsT <DateTime_Run,DateTime_Stream>::loadDefaults(){
//	const U1_byte dpIndexArr[AllDTCnt] =	{0,3,0,2,0,1,0,6, 7,8,10};
	U1_byte ep_index = epD().index();
	logToSD("DateTime::loadDefaults()",ep_index);
	//DateTime_Stream methods:

	//static_cast<DateTime_Stream &>(dStream()).setDPindex(dpIndexArr[ep_index]);

	switch (ep_index) {//										h,m,d,n,y, DP	Current: 5,2,25,5,12
	case 0:	static_cast<DateTime_Stream &>(dStream()).setDTdate(1,2,1,5,12,0); break;// Current: before currentdate
	case 1:	static_cast<DateTime_Stream &>(dStream()).setDTdate(2,3,26,5,12,3); break;// Day off 4hrs from 2:30 tomorrow
	case 2:	static_cast<DateTime_Stream &>(dStream()).setDTdate(0,0,0,0,0,0); break; // Old Normal
	case 3:	static_cast<DateTime_Stream &>(dStream()).setDTdate(5,1,25,5,12,2); break; // Out 1hr from 5:10 today
	case 4:	static_cast<DateTime_Stream &>(dStream()).setDTdate(4,5,4,8,12,0); break; // Normal: after Hol
	case 5:	static_cast<DateTime_Stream &>(dStream()).setDTdate(6,0,3,7,12,1); break; // Hol after currentdate
	//case 6:	static_cast<DateTime_Stream &>(dStream()).setDTdate(0,0,0,0,0,0); break; // Old Normal
	//case 7:	static_cast<DateTime_Stream &>(dStream()).setDTdate(0,0,0,0,0,0); break; // Old Normal
	case 6:	static_cast<DateTime_Stream &>(dStream()).setDTdate(1,2,1,1,12,6); break; // US
	case 7:	static_cast<DateTime_Stream &>(dStream()).setDTdate(1,2,1,1,12,9); break; // Flat
	case 8: static_cast<DateTime_Stream &>(dStream()).setDTdate(1,2,1,1,12,13); break; // DHW
	}
}

template<>
void OperationsT <DailyProg_Run,DailyProg_Stream>::loadDefaults(){
	// Day-Off and In/Out use firstTT as period and E_days as temp. instead of a TT.
	//											0		1			2			3		4	    5		   6		  7		   8		   9	    10		  11		12		    13		  14		  15		 16			17		18,			19
	const char name[AllDPCnt][DLYPRG_N+1] =	{"At-Home",	"Away","DS-Out",  "DS-DayO", "DS-In", "New Prog","At-Home",  "Away", "New Prog","Occupied", "Empty","New Prog","Hols",    "HW-Wk",   "HW-Wk",   "HW-Wk",    "HW-Wk",   "HW-Wk","New Prog","New Prog" };
	const U1_byte fstTT[AllDPCnt] =				{0,			2,		1*8,	   4*8,		 1*8,		0,		  4,	  6,	    0,		  7,	     9,		   0,		 10,	  11,		  13,		  15,		  17,		19,	      0,		 0};
	const U1_byte noTTsA[AllDPCnt] =			{2,			2,		0,		   0,		  0,		0,		  2,	  1,		0,	  	  2,	     1,		   0,	     1,		  2,		  2,		  2,		  2,		3,		  0,		 0};
	const U1_byte dpDaysA[AllDPCnt] =			{127,		127,	10,			1,	     19,	   127,		 127,	 127,	    127,	 127,	     127,	   1,	    127,	  64,		  56,		  4,		  2,		1,		 127,		 0};
	const U1_byte dpTypeA[AllDPCnt] =		{E_dpWeekly,E_dpWeekly,E_dpInOut,E_dpDayOff,E_dpInOut,E_dpNew,E_dpWeekly,E_dpWeekly,E_dpNew,E_dpWeekly,E_dpWeekly, E_dpNew,E_dpWeekly,E_dpWeekly,E_dpWeekly,E_dpWeekly,E_dpWeekly,E_dpWeekly,E_dpNew,E_dpNew};
	const bool dpGrpEnd[AllDPCnt] =				{true,		true,	true,	  true,      true,	   true,	true,	 true,		true,	  true,	     true,	   true,	true,      false,     false,      false,	false,	   true,	  false, false};
	const U1_byte dpRefCountA[AllDPCnt] =		{2,			1,		1,		   1,		  0,		0,		  1,	   0,		 0,		  1,		  0,		0,	     0,	         1,		    1,		  1,		   1,		  1,		0,		0};
	U1_byte ep_index = epD().index();
	logToSD("DailyProg::loadDefaults()",ep_index);

//debugEEprom(NO_OF_ST,NO_COUNT);	
	epD().setName(name[ep_index]);
	setVal(E_firstTT,fstTT[ep_index]);
	setVal(E_noOfTTs,noTTsA[ep_index]);
	setVal(E_dpType,dpTypeA[ep_index] + dpRefCountA[ep_index] );
	setVal(E_days,dpDaysA[ep_index] | (dpGrpEnd[ep_index] ? FULL_WEEK : 0));
	//static_cast<DailyProg_Stream &>(dStream()).loadDays();
}

template<>
void OperationsT <RunT <EpDataT<TMTP_DEF> >,TimeTemp_Stream>::loadDefaults(){
	const U1_byte HRS = 8;
	const U1_byte TMP = 10;
	//									#0 0	1	    #1 2	3		  #6 4	  5		  #7 6	   #9 7	    8	   #10 9  #12 10   #13 11	12	   #14	13	14	    #15 15	16	     #16 17,	18	   #17 19,	20,		21
	const U1_byte serTime[AllTTCnt] =  {7*HRS, 23*HRS,  00*HRS,	SPARE_TT, 7*HRS , 23*HRS, 0*HRS+1, 8*HRS,  22*HRS, 0*HRS, 0*HRS,  8*HRS+3,22*HRS+4,9*HRS+3,21*HRS+4,7*HRS+3,20*HRS+4,6*HRS+3,19*HRS+4,7*HRS+4,22*HRS+4, SPARE_TT};
	const U1_byte TTtemp[AllTTCnt] =   {19+TMP, 17+TMP, 10+TMP,	0		, 15+TMP, 17+TMP, 10+TMP,  20+TMP, 18+TMP, 10+TMP, 10+TMP, 48+TMP, 32+TMP, 49+TMP,  39+TMP,	47+TMP,  38+TMP, 46+TMP, 37+TMP  , 47+TMP, 31+TMP,	0   };
	U1_byte ep_index = epD().index(); 
	logToSD("TimeTemp::loadDefaults()",ep_index);

	setVal(E_ttTime, serTime[ep_index]);	
	setVal(E_ttTemp, TTtemp[ep_index]);	
}