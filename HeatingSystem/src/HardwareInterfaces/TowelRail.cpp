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
		if (!_mixValveController->amControlZone(sharedZoneCallTemp(), TOWEL_RAIL_FLOW_TEMP, _relay->recordID())) {
			_relay->set(needHeat); // too cool
		}
		return needHeat;
	}

	//// Private Methods

	uint8_t TowelRail::sharedZoneCallTemp() const { // returns max shared request temp.
		uint8_t maxTemp = _callFlowTemp;
		for (auto & towelRail : _temperatureController->towelRailArr) {
			if (towelRail.relayPort() == relayPort()) { // got this or shared zone
				if (towelRail._callFlowTemp > maxTemp) maxTemp = towelRail._callFlowTemp;
			}
		}
		return maxTemp;
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

