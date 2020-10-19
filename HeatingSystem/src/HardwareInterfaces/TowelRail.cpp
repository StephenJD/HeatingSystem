#include "TowelRail.h"
#include "A__Constants.h"
#include "..\Assembly\TemperatureController.h"

using namespace Assembly;

namespace HardwareInterfaces {

	void TowelRail::initialise(int towelRailID, UI_TempSensor & callTS, UI_Bitwise_Relay & callRelay, uint16_t onTime, TemperatureController & temperatureController, MixValveController & mixValveController) {
		_callTS = &callTS;
		_mixValveController = &mixValveController;
		_relay = &callRelay;
		_onTime = onTime;
		_temperatureController = &temperatureController ;
		_recordID = towelRailID;
	}	
	
	uint8_t TowelRail::relayPort() const { return _relay->port(); }
	uint16_t TowelRail::timeToGo() const { 
		return _timer > TWL_RAD_RISE_TIME ? _timer : 0; 
	}

	bool TowelRail::check() {
		// En-suite and Family share a zone / relay, so need to prevent the OFF TR disabling the ON one.
		bool needHeat = setFlowTemp();
		if (_callFlowTemp >= sharedZoneCallTemp()) { // required to stop OFF zone taking control from shared ON zone, but if all OFF must process this.
			if (_mixValveController->amControlZone(_callFlowTemp, TOWEL_RAIL_FLOW_TEMP, _relay->recordID())) { // I am controlling zone, so set flow temp
			}
			else { // not control zone
				_relay->set(needHeat); // too cool
			}
		}
		return needHeat;
	}

	//// Private Methods

	uint8_t TowelRail::sharedZoneCallTemp() const { // returns other callTemp if shared, otherwise returns 0.
		uint8_t otherTemp = 0;
		for (auto & towelRail : _temperatureController->towelRailArr) {
			if (&towelRail != this && towelRail.relayPort() == relayPort()) { // got this or shared zone
				if (towelRail._callFlowTemp > otherTemp) otherTemp = towelRail._callFlowTemp;
			}
		}
		return otherTemp;
	}

	bool TowelRail::setFlowTemp() { // returns true if ON
		//_temperatureController->towelRailArr[record()].
		if (_timer > 0) {
			//logger() << L_time << "TowelR " << record() << " tick: " << _timer << L_endl;
			--_timer; // called every second
		}
		_callFlowTemp = 0;

		if (_timer > TWL_RAD_RISE_TIME || rapidTempRise()) {
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
		if (_callTS->hasError()) return false;

		if (_prevTemp == 0) _prevTemp = hotWaterFlowTemp;

		if (abs(hotWaterFlowTemp - _prevTemp) > 1) {
			if (_timer == 0) {
				logger() << L_time << "TowelR " << record() << " TempDiff. Timer set to 60. Was: " << _prevTemp << " now: " << hotWaterFlowTemp << L_endl;
				_timer = TWL_RAD_RISE_TIME;
				_prevTemp = hotWaterFlowTemp;
			}
			else { // _timer running
				if (hotWaterFlowTemp > _prevTemp + TWL_RAD_RISE_TEMP) {
					_timer = _onTime * 60;
					logger() << L_time << "TowelR " << record() << " RapidRise from: " << _prevTemp << " to " << hotWaterFlowTemp << L_endl;
					return true;
				}
				else if (hotWaterFlowTemp < _prevTemp) {
					logger() << L_time << "TowelR " << record() << " Cooled to " << hotWaterFlowTemp << L_endl;
					_prevTemp = hotWaterFlowTemp;
				}
			}
		}
		//_timer = _onTime * 60;
		return false;
	}

}

