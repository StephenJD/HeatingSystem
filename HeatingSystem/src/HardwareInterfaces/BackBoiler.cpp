#include "BackBoiler.h"
#include "Temp_Sensor.h"
#include "Relay.h"

namespace HardwareInterfaces {
	BackBoiler::BackBoiler(I2C_Temp_Sensor & flowTS, I2C_Temp_Sensor & thrmStrTS, Relay & relay)
		: _flowTS(&flowTS)
		, _thrmStrTS(&thrmStrTS)
		, _relay(&relay)
	{}

	bool BackBoiler::check() { // start or stop pump as required
		bool pumpOn = false;
		if (I2C_Temp_Sensor::hasError()) {
			pumpOn = true;
		}
		else {
			auto flowTemp = _flowTS->get_temp();
			auto storeTemp = _thrmStrTS->get_temp();
			if ((flowTemp >= _minFlowTemp) && (flowTemp >= storeTemp + _minTempDiff)) {
				pumpOn = true;
			}
		}
		_relay->setRelay(pumpOn);
		return pumpOn;
	}
}