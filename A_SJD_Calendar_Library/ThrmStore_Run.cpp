#include "ThrmStore_Run.h"
#include "D_Factory.h"
#include "TempSens_Run.h"
#include "MixValve_Run.h"
#include "Zone_Run.h"
#include "Event_Stream.h"

ThrmSt_Run::ThrmSt_Run() : _currDeliverTemp(45), _groundT(15), _isHeating(false) {
}

S2_byte ThrmSt_Run::getFractionalOutsideTemp() const {
	return f->tempSensorR(getVal(OutsideTS)).getFractionalSensTemp();
}

U1_byte ThrmSt_Run::getTopTemp() const {
	U1_byte temp = f->tempSensorR(getVal(OvrHeatTS)).getSensTemp();
	if (temp_sense_hasError == true) temp = f->tempSensorR(getVal(DHWpreMixTS)).getSensTemp();
	return temp;
}

U1_byte ThrmSt_Run::getOutsideTemp() const {
	return f->tempSensorR(getVal(OutsideTS)).getSensTemp();
}

void ThrmSt_Run::setLowestCWtemp(bool isFlowing) {
	if (isFlowing || (_groundT > getGroundTemp())) _groundT = getGroundTemp();
}

U1_byte ThrmSt_Run::getGroundTemp() const {
	return f->tempSensorR(getVal(GroundTS)).getSensTemp();
}

bool ThrmSt_Run::dumpHeat() const {
	return getTopTemp() >= getVal(OvrHeatTemp);
}

bool ThrmSt_Run::check() { // checks zones, twl rads & thm store for heat requirement and sets relays accordingly
	//static U4_byte	lastTick;
	//static U1_byte secsSince10Mins;
	//secsSince10Mins += Curr_DateTime_Run::secondsSinceLastCheck(lastTick);
	//U4_byte startCheck = micros();

	--Zone_Run::z_period;
	U1_byte i;
	for (i=0; i < f->towelRads.count(); ++i) { // takes 200mS, required every second
		f->towelRads[i].run().check(); // setFlowTemp, mixCall & switches relays
	}

	//U4_byte towelRadsTime = micros();

	//if (Zone_Run::z_period == 1 || secsSince10Mins >= 600) {
		//secsSince10Mins == 0;
		for (i=0; i < f->zones.count(); ++i) { // takes 850mS
			// DHW zone checks Mix Valves are hot enough & DHW is OK.
			f->zoneR(i).check(); // setZFlowTemp, mixCall & switches relays
		}
	//}
	//U4_byte zonesTime = micros();

	if (Zone_Run::z_period <= 0) Zone_Run::z_period = AVERAGING_PERIOD;
	
	if(!temp_sense_hasError) {
		for (i = 0; i< f->mixingValves.count(); ++i) { // takes 104mS
			f->mixValveR(i).check(); // Check Mixing Valve flow temps
		}
	}

	//U4_byte mixingValvesTime = micros();

	//logToSD("timeFor_towelRadsTime",(towelRadsTime - startCheck)/1000);
    //logToSD("timeFor_zonesTime",(zonesTime-towelRadsTime)/1000);
    //logToSD("timeFor_mixingValvesTime",(mixingValvesTime-zonesTime)/1000);

	//byte endTempDiff = getcurrTempRequest(index) - f->thermStoreR.getFractionalOutsideTemp();
	//byte warmTime = zp_Data::recordEndData(endTempDiff,currTime.getTime());
	//if (warmTime > 0){
	//	setVal(zAutoFinalTmp,warmTime);
	//}
	return true;
}

bool ThrmSt_Run::needHeat(U1_byte currRequest, U1_byte nextRequest) {
	// called every second
	bool needHeat = false;
	setLowestCWtemp(false);
	// Check temp for each mix valve		
	for (int m = 0; m < f->mixingValves.count(); ++m) {
		// Only call for heat if mixing valve is below temp
		needHeat = needHeat | f->mixValveR(m).needHeat(_isHeating);
	}
	if (!_isHeating && needHeat) logToSD("ThrmSt_Run::needHeat?\tMixValve Needs Heat  Curr DHW temp:",_currDeliverTemp);
	bool tsNeedHeat = dhwNeedsHeat(currRequest, nextRequest);
//std::cout << "ThmStore::NeedHeat?" << '\n';
	if (!_isHeating && tsNeedHeat) logToSD("ThrmSt_Run::needHeat?\tStore Needs Heat  Curr DHW temp:",_currDeliverTemp);
	needHeat = needHeat | tsNeedHeat;
    if (_isHeating && !needHeat) {
		f->eventS().newEvent(EVT_GAS_TEMP,f->tempSensorR(getVal(GasTS)).getSensTemp());
	    logToSD("ThrmSt_Run::needHeat?\tGas Turned OFF   Gas Flow Temp:",f->tempSensorR(getVal(GasTS)).getSensTemp());
	    logToSD("ThrmSt_Run::needHeat?\tCurr DHW temp:",_currDeliverTemp);
	} else if (!_isHeating && needHeat) {
		logToSD("ThrmSt_Run::needHeat?\tGas Turned ON   Curr DHW temp:",_currDeliverTemp);
		logToSD("ThrmSt_Run::needHeat?\tGroundT, Top, Mid, Lower", 0,
		_groundT,
		f->thermStoreR().getTopTemp(),
		f->tempSensorR(f->thermStoreR().getVal(MidDhwTS)).getSensTemp(),
		f->tempSensorR(f->thermStoreR().getVal(LowerDhwTS)).getSensTemp());
	}
	_isHeating = needHeat;
	return needHeat;
}

// Private methods

void ThrmSt_Run::calcCapacities() {
	// Using conductionConstant, flowRate & coilFraction calculate exponent k at each level
	// Actual Data without gas-reheat on shallow bath: Top:-3C, Mid:-8C, Bot:-4C.
	U1_byte flowRate = getVal(DHWflowRate);
	
	float cond = getVal(Conductivity); //25; //
	float F[4]; // First calc length of Heat Exchanger at level of sensor
	F[3] = ((float)getVal(TopSensHeight) - getVal(LowerSensHeight)); // Length of heat exchanger
	F[0] = ((float)getVal(MidSensHeight) - getVal(LowerSensHeight))/2.0F; // cm's
	F[1] = F[3]/2.0F;
	F[2] = (getVal(TopSensHeight) - getVal(MidSensHeight))/2.0F;

	_bottomV = F[0] * pow((float)getVal(CylDia),2) * 0.00071F ; // vol in litres, allowing 10% for coil volume
	_midV = F[1] * pow((float)getVal(CylDia),2) * 0.00071F ; // vol in litres
	_upperV = (getVal(CylHeight) - (getVal(TopSensHeight) + getVal(MidSensHeight))/2) * pow((float)getVal(CylDia),2) * 0.00071F ; // vol in litres
	// Now calc Fraction of heatexchanger at each level
	F[0] = F[0] / F[3];
	F[1] = F[1] / F[3];
	F[2] = F[2] / F[3];
	float k[3];
	k[0]=(0.046F-0.0031F*cond)*F[0]+(.026F * cond-.032F)*F[0]/ pow(float(flowRate/60.0),float(1.0-.001*cond+(.003*cond-.025)*F[0]));
	k[1]=(0.046F-0.0031F*cond)*F[1]+(.026F * cond-.032F)*F[1]/ pow(float(flowRate/60.0),float(1.0-.001*cond+(.003*cond-.025)*F[1]));
	k[2]=(0.046F-0.0031F*cond)*F[2]+(.026F * cond-.032F)*F[2]/ pow(float(flowRate/60.0),float(1.0-.001*cond+(.003*cond-.025)*F[2]));
	
	_bottomC = 1.0F - exp(-k[0]); 
	_midC = 1.0F - exp(-k[1]);
	_upperC = 1.0F - exp(-k[2]);
}

U1_byte ThrmSt_Run::calcCurrDeliverTemp(U1_byte callTemp) const {
	float boilerPwr = getVal(BoilerPower);
	U1_byte duration = getVal(DHWflowTime);

	float topT = getTopTemp();
	float midT = f->tempSensorR(getVal(MidDhwTS)).getSensTemp();
	float botT = f->tempSensorR(getVal(LowerDhwTS)).getSensTemp();

	// using Capacities, ground and store temps, calc HW temp at each level
	float HWtemp[3];
	HWtemp[0] = _groundT + (botT- _groundT) * _bottomC;
	HWtemp[1] = HWtemp[0] + (midT- HWtemp[0]) * _midC;
	HWtemp[2] = HWtemp[1] + (topT- HWtemp[1]) * _upperC;
	// Using HWtemps, calculate share of energy for each section of store
	float share[3];
	share[0] = (HWtemp[0] - _groundT)/(HWtemp[2] - _groundT);
	share[1] = (HWtemp[1] - HWtemp[0])/(HWtemp[2] - _groundT);
	share[2] = (HWtemp[2] - HWtemp[1])/(HWtemp[2] - _groundT);
	// Calc final store temps using ratio of DHW vol to storeVol
	float storeTemps[3];
	float factor = ((callTemp - _groundT) * getVal(DHWflowRate) * .07F - boilerPwr) * duration *1.43F;
	storeTemps[0] = botT - factor * share[0] / _bottomV;
	storeTemps[1] = midT - factor * share[1] / _midV;
	storeTemps[2] = topT - factor * share[2] / _upperV;

	// Calc final HW temps
	HWtemp[0] = _groundT + (storeTemps[0] - _groundT) * _bottomC;
	HWtemp[1] = HWtemp[0] + (storeTemps[1] - HWtemp[0]) * _midC;
	HWtemp[2] = HWtemp[1] + (storeTemps[2] - HWtemp[1]) * _upperC + 0.5f;
	return (U1_byte) HWtemp[2];
}

bool ThrmSt_Run::dhwNeedsHeat(U1_byte callTemp, U1_byte nextRequest) {
	_currDeliverTemp = calcCurrDeliverTemp(nextRequest > callTemp ? nextRequest : callTemp);
	// Note that because callTemp determins the rate of heat-extraction, currDeliverTemp, which is temperature AFTER fillimng a bath, will change if callTemp changes.
	static bool hasRequestedCondReduction = false;
	bool dhwTempOK = dhwDeliveryOK(callTemp);
	if (!hasRequestedCondReduction && _currDeliverTemp >= callTemp && !dhwTempOK) { // reduce conductivity if claims to be hot enought, but isn't
		hasRequestedCondReduction = true;
		Serial.println("wants to reduce conductivity");
		//f->eventS().newEvent(EVT_THS_COND_CHANGE,S1_byte(getVal(Conductivity)) - 1);
	    logToSD("ThrmSt_Run::dhwNeedsHeat\tDHW too cool-reduce cond?\tCond, call, next,DHW,dhwFlowTemp,DHWpreMixTS:",0,0,S1_byte(getVal(Conductivity)),callTemp,nextRequest,_currDeliverTemp,f->tempSensorR(getVal(DHWFlowTS)).getSensTemp(),f->tempSensorR(getVal(DHWpreMixTS)).getFractionalSensTemp()/256);
		//setVal(Conductivity, U1_byte(getVal(Conductivity)) - 1);
	}
	if (dhwTempOK) hasRequestedCondReduction = false;
	U1_byte addHysteresis = 0;
	if (!f->occasionalHeaters[0].run().check()) { // only add hysteresis if stove not on.
		if (_isHeating) addHysteresis = THERM_STORE_HYSTERESIS;
	}
	return _currDeliverTemp < (callTemp + addHysteresis);	// If HWtemp[2] is not hot enough, we need heat.
}

bool ThrmSt_Run::dhwDeliveryOK(U1_byte currRequest) const {
	/*
	Compare DHW temp each side of thermostatic valve to see if cold is being mixed in.
	If temps are the same, tank is too cool.
	*/
	S1_byte dhwFlowTemp = f->tempSensorR(getVal(DHWFlowTS)).getSensTemp();
	bool tempError = temp_sense_hasError;
	S1_byte DHWpreMixTemp = f->tempSensorR(getVal(DHWpreMixTS)).getSensTemp();
	tempError = tempError | temp_sense_hasError;
	if (DHWpreMixTemp < 0 || dhwFlowTemp - DHWpreMixTemp > 2) tempError = true;
	//int i = 5;
	//while (DHWpreMixTemp - dhwFlowTemp < -5 && --i>0) {
	//	logToSD("ThrmSt_Run::dhwDeliveryOK\t TempError, DHWFlow, DHWpre:",tempError,dhwFlowTemp,DHWpreMixTemp);
	//	dhwFlowTemp = f->tempSensorR(getVal(DHWFlowTS)).getSensTemp();
	//	tempError = temp_sense_hasError;
	//	DHWpreMixTemp = f->tempSensorR(getVal(DHWpreMixTS)).getSensTemp();
	//	tempError = tempError | temp_sense_hasError;
	//}
	return (tempError || dhwFlowTemp >= currRequest || DHWpreMixTemp > dhwFlowTemp);
}