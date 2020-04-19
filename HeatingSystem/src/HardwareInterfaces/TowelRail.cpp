#include "TowelRail.h"
#include "A__Constants.h"
#include "..\Assembly\TemperatureController.h"

using namespace Assembly;

namespace HardwareInterfaces {

	void TowelRail::initialise(int towelRailID, I2C_Temp_Sensor & callTS, Bitwise_Relay & callRelay, TemperatureController & temperatureController, MixValveController & mixValveController, int8_t maxFlowTemp) {
		_callTS = &callTS;
		_mixValveController = &mixValveController;
		_relay = &callRelay;
		_temperatureController = &temperatureController ;
		_recordID = towelRailID;
		_maxFlowTemp = maxFlowTemp;
	}	
	
	bool TowelRail::isCallingHeat() const {
		bool needHeat = false;

		//needHeat = setFlowTemp();
		//U1_byte myMixV = getVal(tr_MixValveID);
		//U1_byte callTemp = sharedZoneCallTemp(zoneRelay);
		//if (_callFlowTemp > callTemp) callTemp = _callFlowTemp; // required to stop OFF zone taking control from shared ON zone

		//if (f->mixValveR(myMixV).amControlZone(callTemp, getVal(tr_MaxFlowT), zoneRelay)) { // I am controlling zone, so set flow temp
		//}
		//else { // I can not set the flow temp, so just set the call relay
		//	// If turning relay off, check any zones sharing call relay (e.g.towel rad circuits) also want off.
		//	if (needHeat || sharedZoneCallTemp(zoneRelay) > MIN_FLOW_TEMP) f->relayR(zoneRelay).set(needHeat);
		//}
		return needHeat;
	}

	//// Private Methods

	//U1_byte TowelR_Run::sharedZoneCallTemp(U1_ind zoneRelay) const { // returns other callTemp if shared, otherwise returns 0.
	//	U1_byte result = 0;
	//	for (U1_byte i = 0; i < f->towelRads.count(); i++) {
	//		if (i != epDI().index() && f->towelRads[i].getVal(zCallRelay) == zoneRelay) { // got shared zone
	//			return static_cast<TowelR_Run &>(f->towelRads[i].run())._callFlowTemp;
	//		}
	//	}
	//	return result;
	//}

	bool TowelRail::setFlowTemp() { // returns true if ON
		if (_timer > 0) --_timer; // called every second
		_callFlowTemp = 0;

		//if (f->thermStoreR().dumpHeat()) {// turn on to dump heat
		//	_callFlowTemp = TOWEL_RAIL_FLOW_TEMP;
		//}
		//else 
		if ((_timer > TWL_RAD_RISE_TIME) || rapidTempRise()) {
			// Don't fire TowelRads if outside temp close to room temp (e.g.summer!)
			if (_temperatureController->outsideTemp() < 19) {
				_callFlowTemp = TOWEL_RAIL_FLOW_TEMP;
			}
		}
		return (_callFlowTemp == TOWEL_RAIL_FLOW_TEMP);
	}

	bool TowelRail::rapidTempRise() const {
		// If call-sensor increases by at least 5 degrees in 10 seconds, return true.
		auto hotWaterFlowTemp = _callTS->get_temp();
		if (!_callTS->hasError()) {
			if (_prevTemp == 0) _prevTemp = hotWaterFlowTemp;

			if (abs(hotWaterFlowTemp - _prevTemp) > 1) {
				//auto index = epD().index();
				if (_timer == 0) {
					_timer = TWL_RAD_RISE_TIME;
					_prevTemp = hotWaterFlowTemp;
					logger() << L_time << "TempDiff. Timer 0; Reset prevTemp, set timer to rise-time. Temp is: " << hotWaterFlowTemp << L_endl;
				}
				else { // _timer running
					if (hotWaterFlowTemp > _prevTemp + TWL_RAD_RISE_TEMP) {
						//logger() << "TowelR_Run::rapidTempRise\tTowelRad RapidRise was:" << _prevTemp << epD().getName());
						//_timer = getVal(tr_RunTime) * 60; // turn-on time in secs
						//_prevTemp = hotWaterFlowTemp;
						//f->thermStoreR().setLowestCWtemp(true); // Reset _groundT for therm-store heat calculation because water is being drawn.
						//logger() << "TowelR_Run::rapidTempRise\tTowelRad RapidRise is: " << hotWaterFlowTemp << " GroundT: ", f->thermStoreR().getGroundTemp());
						return true;
					}
					else if (hotWaterFlowTemp < _prevTemp) {
						Serial.println("Cool; Reset prevTemp");
						_prevTemp = hotWaterFlowTemp;
					}
				}
			}
		}
		return false;
	}

}

