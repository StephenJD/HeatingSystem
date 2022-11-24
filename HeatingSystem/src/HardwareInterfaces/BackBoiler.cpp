#include "BackBoiler.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"

namespace HardwareInterfaces {
	BackBoiler::BackBoiler(UI_TempSensor & flowTS, UI_TempSensor & thrmStrTS, UI_Bitwise_Relay & relay)
		: _flowTS(&flowTS)
		, _thrmStrTS(&thrmStrTS)
		, _relay(&relay)
	{}

	bool BackBoiler::check() { // start or stop pump as required
		bool pumpOn = false;
		if (_flowTS->hasError()) {
			pumpOn = true;
			logger() << "BackB FlowTS error" << L_endl;
		}
		else {
			auto flowTemp = _flowTS->get_temp();
			auto storeTemp = _thrmStrTS->get_temp();
			logger() << "BackB FlowTS: " << flowTemp << " ReturnT: " << storeTemp << L_endl;
			if ((flowTemp >= _minFlowTemp) && (flowTemp >= storeTemp + _minTempDiff)) {
				pumpOn = true;
			}
		}
		_relay->set(pumpOn);
		return pumpOn;
	}

	bool BackBoiler::isOn() const { return _relay->logicalState(); }

}