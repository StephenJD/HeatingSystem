#include "TowelR_Run.h"
#include "D_Factory.h"
#include "MixValve_Run.h"
#include "ThrmStore_Run.h"
#include "TempSens_Run.h"
#include "Relay_Run.h"
#include "Event_Stream.h"

TowelR_Run::TowelR_Run() : _callFlowTemp(0), _timer(0), _prevTemp(0) {}

bool TowelR_Run::check() {
	bool needHeat = false;
	U1_byte zoneRelay = getVal(tr_CallRelay);
	if(temp_sense_hasError) {
		f->relayR(zoneRelay).setRelay(needHeat);
	} else {
		needHeat = setFlowTemp();
		U1_byte myMixV = getVal(tr_MixValveID);
		U1_byte callTemp = sharedZoneCallTemp(zoneRelay);
		if (_callFlowTemp > callTemp) callTemp = _callFlowTemp; // required to stop OFF zone taking control from shared ON zone
		
		if (f->mixValveR(myMixV).amControlZone(callTemp,getVal(tr_MaxFlowT),zoneRelay)) { // I am controlling zone, so set flow temp
		} else { // I can not set the flow temp, so just set the call relay
			// If turning relay off, check any zones sharing call relay (e.g.towel rad circuits) also want off.
			if (needHeat || sharedZoneCallTemp(zoneRelay) > MIN_FLOW_TEMP) f->relayR(zoneRelay).setRelay(needHeat);
		}
	}
	return needHeat;
}

// Private Methods

U1_byte TowelR_Run::sharedZoneCallTemp(U1_ind zoneRelay) const { // returns other callTemp if shared, otherwise returns 0.
	U1_byte result = 0;
	for (U1_byte i=0; i < f->towelRads.count(); i++) {
		if (i != epDI().index() && f->towelRads[i].getVal(zCallRelay) == zoneRelay){ // got shared zone
			return static_cast<TowelR_Run &>(f->towelRads[i].run())._callFlowTemp;
		}
	}
	return result;
}

bool TowelR_Run::setFlowTemp() { // returns true if ON
	if (_timer > 0) --_timer; // called every second
	_callFlowTemp = 0;	
	
	if (f->thermStoreR().dumpHeat()) {// turn on to dump heat
		_callFlowTemp = TOWEL_RAIL_FLOW_TEMP;
	} else if ((_timer > TWL_RAD_RISE_TIME) || rapidTempRise()) {
		// Don't fire TowelRads if outside temp close to room temp (e.g.summer!)
		if (f->tempSensorR(OS).getSensTemp() < 19) {
			_callFlowTemp = TOWEL_RAIL_FLOW_TEMP;
		}
	}
	return (_callFlowTemp == TOWEL_RAIL_FLOW_TEMP);
}

bool TowelR_Run::rapidTempRise() const {
	// If call-sensor increases by at least 5 degrees in 10 seconds, return true.
	U1_byte hotWaterFlowTemp = getCallSensTemp();
	if (!temp_sense_hasError) {
		if (_prevTemp == 0) _prevTemp = hotWaterFlowTemp;

		if (abs(hotWaterFlowTemp - _prevTemp) > 1) {
			U1_ind index = epD().index();
			if (_timer == 0) {
				 _timer = TWL_RAD_RISE_TIME;
				_prevTemp = hotWaterFlowTemp;
				logToSD("TempDiff. Timer 0; Reset prevTemp, set timer to rise-time. Temp is: ",hotWaterFlowTemp);
			} else { // _timer running
				if (hotWaterFlowTemp > _prevTemp + TWL_RAD_RISE_TEMP) {
					logToSD("TowelR_Run::rapidTempRise\tTowelRad RapidRise was:",S1_byte(_prevTemp),epD().getName());
					_timer = getVal(tr_RunTime) * 60; // turn-on time in secs
					_prevTemp = hotWaterFlowTemp;
					f->thermStoreR().setLowestCWtemp(true); // Reset _groundT for therm-store heat calculation because water is being drawn.
					logToSD("TowelR_Run::rapidTempRise\tTowelRad RapidRise is: ",S1_byte(hotWaterFlowTemp)," GroundT: ", f->thermStoreR().getGroundTemp());
					return true;
				} else if (hotWaterFlowTemp < _prevTemp) {
					Serial.println("Cool; Reset prevTemp");
					_prevTemp = hotWaterFlowTemp;
				}
			}
		}
	}
	return false;
}

S1_byte TowelR_Run::getCallSensTemp() const {
	return f->tempSensorR(getVal(tr_CallTempSensr)).getSensTemp();
}