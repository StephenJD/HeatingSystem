#include "debgSwitch.h"
#include "D_Factory.h"
#include "EEPROM_Utilities.h"

#include "Relay_Run.h"
#include "MixValve_Run.h"
#include "MixValve_Stream.h"

#include "TempSens_Run.h"
#include "TempSens_Stream.h"

#include "ThrmStore_Stream.h"

#include "Zone_Run.h"
#include "Zone_Stream.h"

#include "TowelR_Stream.h"
#include "TowelR_Run.h"
#include "TowelR_Operations.h"

#include "TimeTemp_Stream.h"

#include "DailyProg_Stream.h"
#include "ThrmStore_Run.h"
#include "OccasHtr_Run.h"
#include "Display_Run.h"
#include "IniSet_NoOf_Stream.h"
#include "DateTime_Stream.h"
#include "I2C_Helper.h"
#include "Event_Stream.h"
#include "Display_Stream.h"
#include "B_Displays.h"
#include "B_Pages.h"
#include "B_Fields.h"
#include "A_Collection.h"

using namespace EEPROM_Utilities;
class Main_display;
class Remote_display;
extern I2C_Helper * rtc;
//extern Displays * main_display;
//extern Displays * hall_display;

#if defined (ZPSIM)
	DTtype CURR_TIME(12,5,25,5,2);
#endif

DTtype currentTime(bool set);
CurrentDateTime_Stream & currTime();
void receiveI2CEvent(int howMany); // defined in .ino
////////// EEPRom allocation //////////////////////////////////
///////////////////////////////////////////////////////////////
/*
0-5  : version
6-15 : One byte each for numbers of 10 objects (Assume only one ThermalStore). I.e. the noOf data is here.
	   {noDisplays, noTempSensors, noRelays, noMixValves, noOccasionalHts, noZones, noTowelRadCcts, noAllDPs, noAllDTs, noAllTTs,NO_COUNT};
16-37: 2-bytes each for location of 11 object data starts, since there may be gaps between the actual object data blocks :
38-41: current time (4 bytes) // to remember time during resets, assuming no separate clock chip.
42...: object data, in same order.

ZonePerformanceData 30 bytes per zone at top of EEProm

*//////////////////////////////////////////////////////////////
//   Constants giving location of object data-start records  //
//   Do not need to be in same order as noOf enum            //
///////////////////////////////////////////////////////////////

/*
enum StartAddr {INI_ST,
				NO_OF_ST		= 6,
				NO_OF_DTA		= NO_OF_ST + 2, // = 8
				THM_ST_ST		= NO_OF_DTA + NO_COUNT, // = 18
				DISPLAY_ST		= THM_ST_ST+2,
				RELAY_ST		= DISPLAY_ST+2, // = 22
				MIXVALVE_ST		= RELAY_ST+2,
				TEMP_SNS_ST		= MIXVALVE_ST+2,
				OCC_HTR_ST		= TEMP_SNS_ST+2,
				ZONE_ST			= OCC_HTR_ST+2,
				TWLRAD_ST		= ZONE_ST+2, // = 32
				DATE_TIME_ST	= TWLRAD_ST+2,
				DAYLY_PROG_ST	= DATE_TIME_ST+2, // =36
				TIME_TEMP_ST	= DAYLY_PROG_ST+2}; // = 38
*/

///////// Global Objects & Groups ///////////////

//FactoryObjects * f(0);

FactoryObjects * getFactory(FactoryObjects * fact) {
	static FactoryObjects * global_Fact = 0;
	if (fact != 0) global_Fact = fact;
	return global_Fact;
}

void setFactory(FactoryObjects * fact) {
	DataStream::f = fact; // 91 lines. Crash after Main_display Constructor, Set Factory Display(0)
	Run::f = fact; // 91 lines. With above,	Crash after Main_display Constructor, Set Factory Display(0)
	getFactory(fact); // 108 lines. With above, Crash after Keypad Base Done. Plugged in crashes during Firr8
	Pages::f = fact;    // 193 lines. With above, Crash after ThrmSt_Run::needHeat?   GroundT/Top/Mid/Lower0  15      64	
	Displays::f = fact; // 230 lines. With above, Crash after ThrmSt_Run::needHeat?   GroundT/Top/Mid/Lower0  15      64
	Fields_Base::f = fact; // 231 run. no crash, full display.
	EpManager::f = fact; // 231 lines,no crash, full display.
}

static Factory * fact(0);

Factory::Factory() 
	: numberOf(new GroupOfOne <OperationsT <RunT <NoOf_EpD>,NoOf_Stream> >(EEPROM_OK)),
	iniSetup(0),
	thermStore(0),
	displays(0),
	relays(0),
	mixingValves(0),
	tempSensors(0),
	occasionalHeaters(0),
	zones(0),
	towelRads(0),
	dateTimes(0),
	dailyPrograms(0),
	timeTemps(0),
	infoMenu(0),
	events(0)
	{logToSD("\nFactory Constructor");}

Factory::~Factory() { // not really required, as only executes at exit from program.
	delete numberOf;
	delete iniSetup;
	delete displays;
	delete thermStore;
	delete relays;
	delete mixingValves;
	delete tempSensors;
	delete occasionalHeaters;
	delete zones;
	delete towelRads;
	delete dateTimes;
	delete dailyPrograms;
	delete timeTemps;
	delete infoMenu;
	delete events;
}

void Factory::createObjects(S1_byte loadDefaults) {
logToSD("Factory::createObjects");
	iniSetup = new GroupOfOne <OperationsT <RunT <EpDataT<INI_DEF> >,IniSet_Stream> >(EEPROM_OK);

	displays = new GroupT<OperationsT <Display_Run, Display_Stream > >(noOf(noDisplays), loadDefaults);

	thermStore = new GroupOfOne<OperationsT <ThrmSt_Run, ThrmStore_Stream> >(loadDefaults);

	relays = new GroupT<OperationsT <Relay_Run, DataStreamT<EpDataT<RELAY_DEF>, Relay_Run, Relay_EpMan, RL_COUNT> > >(noOf(noRelays), loadDefaults);

	mixingValves = new GroupT<OperationsT <MixValve_Run, MixValve_Stream > >(noOf(noMixValves), loadDefaults);

	tempSensors = new GroupT<OperationsT <TempSens_Run,TempSens_Stream> >(noOf(noTempSensors), loadDefaults);

	occasionalHeaters = new GroupT<OperationsT <OccasHtr_Run, DataStreamT<EpDataT<OCCHTR_DEF>, OccasHtr_Run, OccasHtr_EpMan, OH_COUNT> > >(noOf(noOccasionalHts), loadDefaults);

	zones = new GroupT<OperationsT <Zone_Run, Zone_Stream> >(noOf(noZones), loadDefaults);

	towelRads = new GroupT<TowelR_Operations>(noOf(noTowelRadCcts), loadDefaults);

	dailyPrograms = new GroupT<OperationsT <DailyProg_Run,DailyProg_Stream> >(noOf(noAllDPs), loadDefaults);

	dateTimes = new GroupT<OperationsT <DateTime_Run,DateTime_Stream> >(noOf(noAllDTs), loadDefaults);

	timeTemps = new GroupT<OperationsT <RunT <EpDataT<TMTP_DEF> >,TimeTemp_Stream> >(noOf(noAllTTs), loadDefaults);
	
	infoMenu = new GroupOfOne <OperationsT <RunT <EpDataT<INFO_DEF> >,InfoMenu_Stream> >(EEPROM_OK);
	events = new GroupOfOne <OperationsT <RunT <EpDataT<EVENT_DEF> >,Events_Stream> >(EEPROM_OK);
logToSD("Done Factory::createObjects");
}

char *Factory::getVersion(){
	return getString(0,VERSION_SIZE);
}

void Factory::setVersion(const char *newVersion){
	setString(0,newVersion,VERSION_SIZE);
}

U1_byte Factory::noOf(noItemsVals myObject){
	Group & noOfG = *(fact->numberOf);
	EpData & noOfEd = noOfG[0].epD();
	NoOf_EpD & noOfRef = static_cast<NoOf_EpD &>(noOfEd);
	return noOfRef(myObject);
}

bool Factory::initialiseProgrammer() { // called by Displays() base-class constructor to ensure initialisation before concrete displays created.
	logToSD("Factory Ini");
	i2C = new I2C_Helper_Auto_Speed<27>(Wire, ZERO_CROSS_PIN, NO_OF_RETRIES, ZERO_CROSS_DELAY, resetI2C, 10000);
	//i2C->setAsMaster(I2C_MASTER_ADDR);
	//i2C->onReceive(receiveI2CEvent);
	fact = new Factory(); // deleted at exit of program

	if (rtc->notExists(EEPROM_ADDRESS)) return false;
	Serial.print("OldV:"); Serial.print(getVersion()); Serial.println(": NewV:" VERSION ":");
	if (strcmp(getVersion(),VERSION) != 0) { // Change of eeprom layout, so reload defaults
		(*(fact->numberOf))[0].loadDefaults();
		fact->createObjects(RELOAD_EEPROM);
		setVersion(VERSION);
	} else {
		fact->createObjects(EEPROM_OK);
	}

	FactoryObjects * factObj = fact->createFactObject();

	DateTime_Stream::loadAllDTs(*factObj);

	factObj->thermStoreR().calcCapacities();

	setFactory(factObj);
	//U1_byte errCode = 0;
	//if (errCode = getMixArduinoFailed(*i2C,MIX_VALVE_I2C_ADDR)) {
	//	hardReset_Performed(*i2C, MIX_VALVE_I2C_ADDR);
	//} else {
	//	for (int i=0; i<noOf(noMixValves); ++i) {
	//		errCode |= factObj->mixValveR(i).sendSetup();
	//	}
	//}
	//if (errCode) {
	//	logToSD("MixValve Arduino Setup failed");
	//} else logToSD("MixValve Arduino Setup OK");

	for (int i=0; i<noOf(noZones); ++i) {
		factObj->zoneR(i).refreshCurrentTT(*factObj);
	}

	logToSD("FIRR 1");
	return true;
}

FactoryObjects * Factory::createFactObject() {
	logToSD("create FactObject ..");
	FactoryObjects * f = getFactory();
	if (f) {
		delete f;
	}
	f = new FactoryObjects( // deleted at exit of program
		*numberOf, 
		*iniSetup, 
		*thermStore, 
		*displays, 
		*relays, 
		*mixingValves, 
		*tempSensors,
		*occasionalHeaters,
		*zones,
		*towelRads,
		*dateTimes,
		*dailyPrograms,
		*timeTemps,
		*infoMenu,
		*events);
	return f;
}

// ***********************************************************************************

Group & FactoryObjects::refreshTTs() {
	logToSD("FactoryObjects::refreshTTs()");
	delete fact->timeTemps;
	fact->timeTemps = new GroupT<OperationsT <RunT <EpDataT<TMTP_DEF> >,TimeTemp_Stream> >(fact->noOf(noAllTTs), EEPROM_OK);
	setFactory(fact->createFactObject());
	return *fact->timeTemps;
}

Group & FactoryObjects::refreshDPs() {
	logToSD("FactoryObjects::refreshDPs()");
	delete fact->dailyPrograms;
	fact->dailyPrograms = new GroupT<OperationsT <DailyProg_Run,DailyProg_Stream> >(fact->noOf(noAllDPs), EEPROM_OK);
	setFactory(fact->createFactObject());
	return *fact->dailyPrograms;
}

Group & FactoryObjects::refreshDTs() {
	logToSD("FactoryObjects::refreshDTs()");
	delete fact->dateTimes;
	fact->dateTimes = new GroupT<OperationsT <DateTime_Run,DateTime_Stream> >(fact->noOf(noAllDTs), EEPROM_OK);
	FactoryObjects * f = fact->createFactObject();
	setFactory(f);
	DateTime_Stream::loadAllDTs(*f);
	return *fact->dateTimes;
}

FactoryObjects::FactoryObjects(
		Group & numberOf, 
		Group & iniSetup, 
		Group & thermStore, 
		Group & displays, 
		Group & relays, 
		Group & mixingValves, 
		Group & tempSensors,
		Group & occasionalHeaters,
		Group & zones,
		Group & towelRads,
		Group & dateTimes,
		Group & dailyPrograms,
		Group & timeTemps,
		Group & infoMenu,
		Group & events) :
		numberOf(numberOf),
		iniSetup(iniSetup), 
		thermStore(thermStore), 
		displays(displays), 
		relays(relays), 
		mixingValves(mixingValves), 
		tempSensors(tempSensors),
		occasionalHeaters(occasionalHeaters),
		zones(zones),
		towelRads(towelRads),
		dateTimes(dateTimes),
		dailyPrograms(dailyPrograms),
		timeTemps(timeTemps),
		infoMenu(infoMenu),
		events(events) {

		#if defined (ZPSIM)
			currTime().setTime(CURR_TIME.getHrs(),CURR_TIME.get10Mins(),CURR_TIME.getDay(),CURR_TIME.getMnth(),CURR_TIME.getYr());
		#endif
		currTime().loadTime();
		}
		
TempSens_Run & FactoryObjects::tempSensorR(U1_byte index){
	return static_cast<TempSens_Run &>(tempSensors[index].run());
}

Relay_Run & FactoryObjects::relayR(U1_byte index){
	return static_cast<Relay_Run &>(relays[index].run());
}

MixValve_Run & FactoryObjects::mixValveR(U1_byte index){
	return static_cast<MixValve_Run &>(mixingValves[index].run());
}

Zone_Run & FactoryObjects::zoneR(U1_byte index){
	return static_cast<Zone_Run &>(zones[index].run());
}

ThrmSt_Run & FactoryObjects::thermStoreR(){
	return static_cast<ThrmSt_Run &>(thermStore[0].run());
}

Zone_Stream & FactoryObjects::zoneS(U1_byte index){
	return static_cast<Zone_Stream &>(zones[index].dStream());
}

DateTime_Stream & FactoryObjects::dateTimesS(U1_byte index){
	return static_cast<DateTime_Stream &>(dateTimes[index].dStream());
}

DailyProg_Stream & FactoryObjects::dailyProgS(U1_byte index){
	return static_cast<DailyProg_Stream &>(dailyPrograms[index].dStream());
}

TimeTemp_Stream & FactoryObjects::timeTempS(U1_byte index){
	return static_cast<TimeTemp_Stream &>(timeTemps[index].dStream());
}

Events_Stream & FactoryObjects::eventS() {
	return static_cast<Events_Stream &>(events[0].dStream());
}

Display_Stream & FactoryObjects::displayS(U1_byte index) {
	//auto & debug = displays[index];
	return static_cast<Display_Stream &>(displays[index].dStream());
}

void NoOf_EpD::loadDefaultStartAddr(){ // placed here for access to constants
	setShort(THM_ST_ST,TIME_TEMP_ST + 2); // ThermalStoreStart = 40
	setShort(DISPLAY_ST,getShort(THM_ST_ST) + TMS_COUNT); // DisplayStart = 40+16 = 56
	setShort(RELAY_ST,getShort(DISPLAY_ST) + DS_COUNT * fact->noOf(noDisplays)); // RelayStart = 56 + 16 = 72
	setShort(MIXVALVE_ST,getShort(RELAY_ST) + (RL_COUNT + RELAY_N) * fact->noOf(noRelays)); // MixValveStartStart = 72 + 36 = 160
	setShort(TEMP_SNS_ST,getShort(MIXVALVE_ST) + (MV_COUNT + MIXV_N) * fact->noOf(noMixValves)); // TempSensStart = 180
	setShort(OCC_HTR_ST,getShort(TEMP_SNS_ST) + (TS_COUNT + TEMPS_N) * fact->noOf(noTempSensors)); // OccHtrsStart = 315
	setShort(ZONE_ST,getShort(OCC_HTR_ST) + (OH_COUNT + OCCHTR_N) * fact->noOf(noOccasionalHts)); // ZoneStart = 329
	setShort(TWLRAD_ST,getShort(ZONE_ST) + (Z_NUM_VALS + ZONE_N + ZONE_A) * fact->noOf(noZones)); // TwrRadStart = 425
	setShort(DATE_TIME_ST,getShort(TWLRAD_ST) + (TR_COUNT + TWLRL_N) * fact->noOf(noTowelRadCcts)); // DTStart = 461
	setShort(DAYLY_PROG_ST,getShort(DATE_TIME_ST) + DT_COUNT * fact->noOf(noAllDTs)); // DPStart = 497
	setShort(TIME_TEMP_ST,getShort(DAYLY_PROG_ST) + (DP_COUNT + DLYPRG_N) * fact->noOf(noAllDPs)); // TTStart = 739
}