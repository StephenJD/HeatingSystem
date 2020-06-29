#include "TemperatureController.h"
#include "..\Client_DataStructures\Data_ThermalStore.h"
#include "..\Client_DataStructures\Data_MixValveControl.h"
#include "..\Client_DataStructures\Data_Zone.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\HardwareInterfaces\A__Constants.h"
#include "..\HardwareInterfaces\I2C_Comms.h"
#include <Timer_mS_uS.h>

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
		, mixValveControllerArr{ recovery }
	{
		int index = 0;
		logger() << F("loadtempSensors...") << L_endl;
		auto tempSensors = db.tableQuery(TB_TempSensor);

		for (Answer_R<R_TempSensor> tempSensor : tempSensors) {
			tempSensorArr[index].initialise(tempSensor.id(), tempSensor.rec().address); // Reads temp
			++index;
			//if (index == 7)
			//	auto a = true;
		}
		logger() << F("loadtempSensors Completed") << L_endl;

		index = 0;
		auto relays = db.tableQuery(TB_Relay);
		for (Answer_R<R_Relay> relay : relays) {
			relayArr[index].initialise(relay.id(), relay.rec().relay_B);
			++index;
		}

		logger() << F("loadRelays Completed") << L_endl;

		Answer_R<R_ThermalStore> thStRec = *db.tableQuery(TB_ThermalStore).begin();
		thermalStore.initialise(thStRec.rec());

		logger() << F("load thermalStore Completed") << L_endl;
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
		
		logger() << F("load mixValveControllerArr Completed") << L_endl;
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
		logger() << F("loadZones Completed") << L_endl;
	}

	void TemperatureController::checkAndAdjust() {
		// once per second
		static auto lastCheck = millis();
		if (secondsSinceLastCheck(lastCheck) == 0) { return; } // Wait for next second.
		for (auto & ts : tempSensorArr) {
			ts.readTemperature();
			//logger() << F("TS: 0x") << L_hex << ts.getAddress() << F_COLON << L_dec << ts.get_temp() << F(" Error? ") << ts.hasError() << L_endl;
			ui_yield();
		}
		if (clock_().seconds() == 0) { // each minute
			checkZones();
			//logger() << L_time << F("checkAndAdjust Zones checked.") << L_endl;
		}
		for (auto & mixValveControl : mixValveControllerArr) {
			mixValveControl.check(); 
			ui_yield(); 
		}
		backBoiler.check();
		//logger() << L_time << F("BackBoiler::check done") << L_endl;
		ui_yield();
		relayController().updateRelays();
		//logger() << L_time << F("RelaysPort::updateRelays done") << L_endl;
		//if (relaysPort.recovery().isUnrecoverable()) { 
		//	logger() << F("Initiating Arduino Reset") << L_endl;
		//	HardReset::arduinoReset(); 
		//};
	}

	void TemperatureController::checkZones() {
		for (auto & zone : zoneArr) { 
			zone.setFlowTemp(); 
			ui_yield();
		}
	}
}