#include "D_EpData.h"
#include "A_Stream_Utilities.h"
#include "EEPROM_Utilities.h"
#include "A_Displays_Data.h"
#include "D_Factory.h"

FactoryObjects * getFactory(FactoryObjects * fact);

using namespace EEPROM_Utilities;
using namespace Utils;

////////////////////////////// Sample Classes /////////////////////////////////////
/*
EpDataT<INI_DEF>	iniSet_EpD;
NoOf_EpD			noOf_EpD;
EpDataT<DISPL_DEF>	display_EpD;
EpDataT<THRMST_DEF>	thrmSt_EpD;
EpDataT<RELAY_DEF>	relay_EpD;
EpDataT<MIXV_DEF>	mixV_EpD;
EpDataT<TEMPS_DEF>	tempSens_EpD;
EpDataT<OCCHTR_DEF>	occasHtr_EpD;
Zone_EpD			zone_EpD;
EpDataT<TWLRL_DEF>	towelR_EpD;
EpDataT<DTME_DEF>	dateTime_EpD;
EpDataT<DLYPRG_DEF>	dailyProg_EpD;
EpDataT<TMTP_DEF>	timeTemp_EpD;
*/

template<>
const char DataStreamT<EpDataT<INI_DEF>, RunT<EpDataT<INI_DEF> >, IniSet_EpMan, INI_COUNT>::setupPrompts[INI_COUNT][13] = { // in same order as Setup.h::enum noItemsVals
		"Display",
		"NosOfItems",
		"ThermStore",
		"MixgValves",
		"Relays",
		"TempSensrs",
		"Zones",
		"TowelRads",
		"OccasHtrs"};
 
template<>
ValRange DataStreamT<EpDataT<INI_DEF>, RunT<EpDataT<INI_DEF> >, IniSet_EpMan, INI_COUNT>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	return DataStream::addValToFieldStr(item, myVal);
}

////////////////////////////// InfoMenu_EpD /////////////////////////////////////
//{SET_UP,I_THRM_STORE,I_ZONES,I_MIX_VALVE,I_TWL_RAD,I_EVENTS,INFO_COUNT};
template<>
const char DataStreamT<EpDataT<INFO_DEF>, RunT<EpDataT<INFO_DEF> >, IniSet_EpMan, INFO_COUNT>::setupPrompts[INFO_COUNT][13] = {
		"Setup",
		"ThermStore",
		"Zones",
		"MixgValves",
		"TowelRads",
		"Events",
		"I2C Comms"};
 
template<>
ValRange DataStreamT<EpDataT<INFO_DEF>, RunT<EpDataT<INFO_DEF> >, IniSet_EpMan, INFO_COUNT>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	return DataStream::addValToFieldStr(item, myVal);
}

////////////////////////////// Events_EpD /////////////////////////////////////
template<>
const char DataStreamT<EpDataT<EVENT_DEF>, RunT<EpDataT<EVENT_DEF> >, IniSet_EpMan, 1>::setupPrompts[1][13] = {};
 
template<>
ValRange DataStreamT<EpDataT<EVENT_DEF>, RunT<EpDataT<EVENT_DEF> >, IniSet_EpMan, 1>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	return DataStream::addValToFieldStr(item, myVal);
}

////////////////////////////// NoOf_EpD /////////////////////////////////////

//void NoOf_EpD::setVal(U1_ind vIndex, U1_byte val){
//	EEPROM->write(NO_OF_ST + vIndex, val);
//}

//U1_byte NoOf_EpD::getVal(U1_ind vIndex) const {
//	return EEPROM->read(NO_OF_ST + vIndex);
//}

U1_byte NoOf_EpD::operator() (noItemsVals myItem) {
	U1_byte retVal = getVal(myItem);
	return retVal;
}

void NoOf_EpD::incrementNoOf(noItemsVals myItem, S1_byte amount) {
	setVal(myItem, getVal(myItem) + amount);
}

template<>
const char DataStreamT<NoOf_EpD, RunT<NoOf_EpD>, NoOf_EpMan, noAllDPs>::setupPrompts[noAllDPs][13] = { // in same order as Setup.h::enum noItemsVals
		"Displays:",
		"TempSensors:",
		"Relays:",
		"MixgValves:",
		"OccasonlHts:",
		"Zones:",
		"TowelRadCct:"};

template<>
ValRange DataStreamT<NoOf_EpD, RunT<NoOf_EpD>, NoOf_EpMan, noAllDPs>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	return DataStream::addValToFieldStr(item, myVal);
}
			
////////////////////////////// Display_EpD /////////////////////////////////////
//
//Display_EpD::Display_EpD(S1_ind zone) {
//	setVal(dsZone,zone);
//}
char * Display_EpD::getName() const {
	char myZone = getVal(dsZone);
	if (myZone >=0) {
		return getFactory()->zones[myZone].epD().getName();
	} else {
		return (char*)"Main";
	}
}

////////////////////////////// Zone_EpD /////////////////////////////////////

//EEProm Data retrieval

char * Zone_EpD::getAbbrev() const {
	return getString(eepromAddr() + getNoOfVals() + getNameSize(), getAbbrevSize());
}

const U1_byte Zone_EpD::getNoOfItems() const {
	if (isHHWzone()) return zMixValveID; else return Z_SET_VALS;
}

U1_byte Zone_EpD::getAutoMode() const { return (getVal(zFlags) & 8) > 0;}
U1_byte Zone_EpD::getGroup() const { return (getVal(zFlags) >> 4) & 3;} // 0-3
U1_byte Zone_EpD::getWeatherSetback() const { return (getVal(zFlags) & 3);} // 0-3

//Data setting

void Zone_EpD::setAbbrev(const char * abbrev) {
	setString(eepromAddr() + getNoOfVals() + getNameSize(), abbrev, getAbbrevSize());
}

void Zone_EpD::setAutoMode(U1_byte mode) {
	U1_byte flags = getVal(zFlags) & (7+16+32+64);
	if (mode) flags = flags | 8;
	setVal(zFlags,flags);
}

void Zone_EpD::setGroup(U1_byte group){
	U1_byte flags = getVal(zFlags) & (7+8+64);
	flags = flags | (group << 4);
	setVal(zFlags,flags);
}

void Zone_EpD::setWeatherSetback(U1_byte prox){
	U1_byte flags = getVal(zFlags) & (4+8+16+32+64+128);
	flags = flags | (prox & 3);
	setVal(zFlags,flags);
}

bool Zone_EpD::isHHWzone() const {
	if (getVal(zMixValveID) == 255) return true; else return false;
}

U2_byte Zone_EpD::getAutoConstant() const {
	// saved as ln(val) * 31 + 0.5
	return U2_byte(exp(getVal(zAutoTimeConst) / 31.));
}

void Zone_EpD::setAutoConstant(U2_byte myVal) {
	// saved as ln(val) * 31 + 0.5
	setVal(zAutoTimeConst, U1_byte(log(double(myVal)) * 31. + 0.5));
}