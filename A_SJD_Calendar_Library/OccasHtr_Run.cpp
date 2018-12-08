#include "OccasHtr_Run.h"
#include "TempSens_Run.h"
#include "Relay_Run.h"
#include "D_Factory.h"

OccasHtr_Run::OccasHtr_Run() {}

bool OccasHtr_Run::check(){ // start or stop pump as required
	bool pumpOn = false;
	if(temp_sense_hasError) {
		pumpOn = true;
	} else {
		U1_byte flowTemp = f->tempSensorR(getVal(FlowTS)).getSensTemp();
		U1_byte storeTemp = f->tempSensorR(getVal(ThrmStrTS)).getSensTemp();
		if ((flowTemp >= getVal(MinFlowTemp)) && (flowTemp >= storeTemp + getVal(MinTempDiff))) {
			pumpOn = true;
		} 
	}
	f->relayR(getVal(OHRelay)).setRelay(pumpOn);
	return pumpOn;
}