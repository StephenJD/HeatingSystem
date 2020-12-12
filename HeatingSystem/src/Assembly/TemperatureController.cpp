#include "TemperatureController.h"
#include "Sequencer.h"
#include "HeatingSystem_Queries.h"
#include "..\Client_DataStructures\Data_ThermalStore.h"
#include "..\Client_DataStructures\Data_MixValveControl.h"
#include "..\Client_DataStructures\Data_Zone.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_TowelRail.h"
#include "..\HardwareInterfaces\A__Constants.h"
#include "..\HardwareInterfaces\I2C_Comms.h"
#include <clock.h>
#include <Timer_mS_uS.h>

void ui_yield();
Logger& zTempLogger();
Logger& mTempLogger();


namespace Assembly {
	using namespace RelationalDatabase;
	using namespace HardwareInterfaces;
	using namespace client_data_structures;
	using namespace I2C_Recovery;

	TemperatureController::TemperatureController(I2C_Recover & recovery, Sequencer& sequencer, unsigned long * timeOfReset_mS) :
		tempSensorArr{ recovery }
		, backBoiler(tempSensorArr[T_MfF], tempSensorArr[T_Sol], relayArr[R_MFS])
		, thermalStore(tempSensorArr, mixValveControllerArr, backBoiler)
		, mixValveControllerArr{ recovery }
	{
		int index = 0;
		logger() << F("loadtempSensors...") << L_endl;
		auto tempSensors = sequencer.queries()._q_tempSensors;

		for (Answer_R<R_TempSensor> tempSensor : tempSensors) {
			tempSensorArr[index].initialise(tempSensor.id(), tempSensor.rec().address); // Reads temp
			++index;
			//if (index == 7)
			//	auto a = true;
		}
		logger() << F("loadtempSensors Completed") << L_endl;

		index = 0;
		auto relays = sequencer.queries()._q_relay;
		for (Answer_R<R_Relay> relay : relays) {
			relayArr[index].initialise(relay.id(), relay.rec().relay_B);
			++index;
		}

		logger() << F("loadRelays Completed") << L_endl;

		Answer_R<R_ThermalStore> thStRec = *sequencer.queries()._rdb->tableQuery(TB_ThermalStore).begin();
		thermalStore.initialise(thStRec.rec());

		logger() << F("load thermalStore Completed") << L_endl;
		index = 0;
		auto mixValveControls = sequencer.queries()._rdb->tableQuery(TB_MixValveContr);
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
		auto zones = sequencer.queries()._q_zones;
		for (Answer_R<R_Zone> zone : zones) {
			zoneArr[index].initialise(zone.id()
				, tempSensorArr[zone.rec().callTempSens]
				, relayArr[zone.rec().callRelay]
				, thermalStore
				, mixValveControllerArr[zone.rec().mixValve]
				, zone.rec().maxFlowTemp
				, sequencer
			);
			++index;
		}
		logger() << F("loadZones Completed") << L_endl;

		index = 0;
		auto towelrails = sequencer.queries()._q_towelRail;
		for (Answer_R<R_TowelRail> towelRail : towelrails) {
			towelRailArr[index].initialise(towelRail.id()
				, tempSensorArr[towelRail.rec().callTempSens]
				, relayArr[towelRail.rec().callRelay]
				, towelRail.rec().onTemp
				, *this
				, mixValveControllerArr[towelRail.rec().mixValve]
			);
			++index;
		}
		logger() << F("loadTowelRails Completed") << L_endl;
#ifndef ZPSIM
		sequencer.recheckNextEvent();
#endif
	}

	void TemperatureController::checkAndAdjust() { // Called every Arduino loop
		// once per second
		static auto lastCheck = millis();
		if (secondsSinceLastCheck(lastCheck) == 0) { return; } // Wait for next second.
		logger().close();
		zTempLogger().close();
		mTempLogger().close();

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

		for (auto & towelRail : towelRailArr) {
			towelRail.check();
			ui_yield(); 
		}

		for ( auto & relay : relayArr) {
			relay.checkControllerStateCorrect();
		}

		relayController().updateRelays();
		//logger() << L_time << F("RelaysPort::updateRelays done") << L_endl;
		if (mixValveControllerArr[0].recovery().isUnrecoverable()) {
			logger() << F("Initiating Arduino Reset") << L_endl;
			HardReset::arduinoReset(); 
		};
	}

	void TemperatureController::checkZones() {
		auto checkForPreHeat = clock_().minUnits() == 0; // each 10 minutes

		for (auto & zone : zoneArr) { 
			zone.setFlowTemp();
			if (checkForPreHeat) zone.preHeatForNextTT();
			ui_yield();
		}
	}
}