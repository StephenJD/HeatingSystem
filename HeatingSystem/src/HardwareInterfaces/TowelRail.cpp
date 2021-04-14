#include "TowelRail.h"
#include "A__Constants.h"
#include "..\Assembly\TemperatureController.h"

using namespace Assembly;
namespace arduino_logger {
	Logger& profileLogger();
}
using namespace arduino_logger;

namespace HardwareInterfaces {

	void TowelRail::initialise(int towelRailID, UI_TempSensor & callTS, UI_Bitwise_Relay & callRelay, uint16_t onTime, uint8_t onTemp, TemperatureController & temperatureController, MixValveController & mixValveController) {
		_callTS = &callTS;
		_mixValveController = &mixValveController;
		_relay = &callRelay;
		_onTime = onTime;
		_onTemp = onTemp;
		_temperatureController = &temperatureController ;
		_recordID = towelRailID;
	}	
	
	uint8_t TowelRail::relayPort() const { return _relay->port(); }
	uint16_t TowelRail::timeToGo() const { 
		return _timer > TWL_RAD_RISE_TIME ? _timer - TWL_RAD_RISE_TIME : 0;
	}

	bool TowelRail::check() {
		// En-suite and Family share a zone / relay, so need to prevent the OFF TR disabling the ON one.
		bool needHeat = false;
		if (_temperatureController->outsideTemp() < 19) { // Don't fire TowelRads if outside temp close to room temp (e.g.summer!)
			needHeat = setFlowTemp();
			if (!_mixValveController->amControlZone(sharedZoneCallTemp(), _onTemp, _relay->recordID())) {
				_relay->set(needHeat); // too cool
			}
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
		auto currFlowTemp = _callTS->get_temp();
		if (_timer > 0) {
			//profileLogger() << L_time << "TowelR " << record() << " tick: " << _timer << " FlowT: " << (int)currFlowTemp << L_endl;
			--_timer; // called every second
			if (_timer == TWL_RAD_RISE_TIME) _timer = 0;
		}
		_callFlowTemp = 0;
		if (_timer > TWL_RAD_RISE_TIME || rapidTempRise(currFlowTemp)) {
			_prevTemp = currFlowTemp;
			_callFlowTemp = _onTemp;
		}
		return (_callFlowTemp == _onTemp);
	}

	bool TowelRail::rapidTempRise(uint8_t hotWaterFlowTemp) const {
		// If call-sensor increases by at least 5 degrees in 10 seconds, return true.
		if (_callTS->hasError()) return false;

		if (_prevTemp == 0) _prevTemp = hotWaterFlowTemp;

		auto tempDiff = hotWaterFlowTemp - _prevTemp;
		// only called if _timer < TWL_RAD_RISE_TIME
		if (tempDiff < 0) {
			profileLogger() << L_time << "TowelR " << record() << " Reset Timer. Cooled to " << hotWaterFlowTemp << L_endl;
			_prevTemp = hotWaterFlowTemp;
			_timer = 0;
		} else if (_timer == 0) {
			if (tempDiff > 1) {
				profileLogger() << L_time << "TowelR " << record() << " TempDiff. Timer set to TWL_RAD_RISE_TIME. Was: " << _prevTemp << " now: " << hotWaterFlowTemp << L_endl;
				_timer = TWL_RAD_RISE_TIME;
				_prevTemp = hotWaterFlowTemp;			
			}
		} else if (tempDiff >= TWL_RAD_RISE_TEMP) {
			_timer = _onTime * 60 + TWL_RAD_RISE_TIME; // stops at TWL_RAD_RISE_TIME.
			profileLogger() << L_time << "TowelR " << record() << " RapidRise from: " << _prevTemp << " to " << hotWaterFlowTemp << L_endl;
			return true;
		}

		return false;
	}

}

