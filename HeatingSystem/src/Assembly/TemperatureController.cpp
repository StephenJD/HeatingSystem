#include "TemperatureController.h"
#include "..\Client_DataStructures\Data_ThermalStore.h"
#include "..\Client_DataStructures\Data_MixValveControl.h"
#include "..\Client_DataStructures\Data_Zone.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\HardwareInterfaces\A__Constants.h"
#include "..\HardwareInterfaces\I2C_Comms.h"
#include <Clock.h>

void ui_yield();

namespace Assembly {
	using namespace RelationalDatabase;
	using namespace HardwareInterfaces;
	using namespace client_data_structures;
	using namespace I2C_Recovery;

	TemperatureController::TemperatureController(I2C_Recover & recovery, RelationalDatabase::RDB<TB_NoOfTables> & db, unsigned long * timeOfReset_mS) :
		tempSensorArr{ recovery }
		, backBoiler(tempSensorArr[T_MfF], tempSensorArr[T_Sol], relayArr[R_MFS])
		, thermalStore(tempSensorArr, mixValveControllerArr, backBoiler)
		, relaysPort{0x7F, recovery, IO8_PORT_OptCoupl, ZERO_CROSS_PIN, RESET_OUT_PIN}
		, mixValveControllerArr{ recovery }
	{
		int index = 0;
		auto tempSensors = db.tableQuery(TB_TempSensor);
		for (Answer_R<R_TempSensor> tempSensor : tempSensors) {
			//logger() << "Initialise TS[]", index, " Rec ID:" , tempSensor.id());

			tempSensorArr[index].initialise(tempSensor.id(), tempSensor.rec().address); // Reads temp
			++index;
			//if (index == 7)
			//	auto a = true;
		}
		logger() << "loadtempSensors Completed" << L_endl;

		index = 0;
		auto relays = db.tableQuery(TB_Relay);
		for (Answer_R<R_Relay> relay : relays) {
			relayArr[index].initialise(relay.id(), relay.rec().port, LOW);
			++index;
		}

		logger() << "loadRelays Completed" << L_endl;

		Answer_R<R_ThermalStore> thStRec = *db.tableQuery(TB_ThermalStore).begin();
		thermalStore.initialise(thStRec.rec());

		index = 0;
		auto mixValveControls = db.tableQuery(TB_MixValveContr);
		for (Answer_R<R_MixValveControl> mixValveControl : mixValveControls) {
			mixValveControllerArr[index].initialise(index
				, MIX_VALVE_I2C_ADDR
				, relayArr
				, tempSensorArr
				, mixValveControl.rec().flowTempSens
				, mixValveControl.rec().storeTempSens
			);
			mixValveControllerArr[index].setResetTimePtr(timeOfReset_mS);
			++index;
		}
		
		index = 0;
		auto zones = db.tableQuery(TB_Zone);
		for (Answer_R<R_Zone> zone : zones) {
			zoneArr[index].initialise(zone.id()
				, tempSensorArr[zone.rec().callTempSens]
				, relayArr[zone.rec().callRelay]
				, thermalStore
				, mixValveControllerArr[zone.rec().mixValve]
				, zone.rec().maxFlowTemp
			);
			++index;
		}
		logger() << "loadZones Completed" << L_endl;
	}

	void TemperatureController::checkAndAdjust() {
		// once per second
		static auto lastCheck = millis();
		if (Clock::secondsSinceLastCheck(lastCheck) == 0) { return; } // Wait for next second.
		for (auto & ts : tempSensorArr) {
			ts.readTemperature();
			//logger() << "TS: 0x" << L_hex << ts.getAddress() << " " << L_dec << ts.get_temp() << " Error? " << ts.hasError() << L_endl;
			ui_yield();
		}
		if (clock_().seconds() == 0) checkZones(); // each minute
		for (auto & mixValveControl : mixValveControllerArr) {mixValveControl.check(); ui_yield(); }
		backBoiler.check();
		ui_yield();
		relaysPort.updateRelays();
		if (relaysPort.recovery().isUnrecoverable()) { 
			logger() << "Initiating Arduino Reset" << L_flush;
			HardReset::arduinoReset(); 
		};
	}

	void TemperatureController::checkZones() {
		for (auto & zone : zoneArr) { zone.setFlowTemp(); ui_yield(); }
	}
}