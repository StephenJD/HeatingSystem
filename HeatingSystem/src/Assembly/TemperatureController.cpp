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
Logger& profileLogger();


namespace Assembly {
	using namespace RelationalDatabase;
	using namespace HardwareInterfaces;
	using namespace client_data_structures;
	using namespace I2C_Recovery;

	TemperatureController::TemperatureController(I2C_Recover & recovery, HeatingSystem_Queries& queries, Sequencer& sequencer, unsigned long * timeOfReset_mS) :
		tempSensorArr{ recovery }
		, backBoiler(tempSensorArr[T_MfF], tempSensorArr[T_Sol], relayArr[R_MFS])
		, thermalStore(tempSensorArr, mixValveControllerArr, backBoiler)
		, mixValveControllerArr{ recovery }
	{
		Zone::setSequencer(sequencer);
		int index = 0;
		logger() << F("loadtempSensors...") << L_endl;

		for (Answer_R<R_TempSensor> tempSensor : queries.q_tempSensors) {
			tempSensorArr[index].initialise(tempSensor.id(), tempSensor.rec().address); // Reads temp
			++index;
			//if (index == 7)
			//	auto a = true;
		}
		logger() << F("loadtempSensors Completed") << L_endl;

		index = 0;
		for (Answer_R<R_Relay> relay : queries.q_relays) {
			relayArr[index].initialise(relay.id(), relay.rec().relay_B);
			++index;
		}
		logger() << F("loadRelays Completed") << L_endl;

		Answer_R<R_ThermalStore> thStRec = *queries.q_ThermStore.begin();
		thermalStore.initialise(thStRec.rec());
		logger() << F("load thermalStore Completed") << L_endl;

		index = 0;
		for (Answer_R<R_MixValveControl> mixValveControl : queries.q_MixValve) {
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
		for (Answer_R<R_Zone> zone : queries.q_Zones) {
			zoneArr[index].initialise(
				zone
				, tempSensorArr[zone.rec().callTempSens]
				, relayArr[zone.rec().callRelay]
				, thermalStore
				, mixValveControllerArr[zone.rec().mixValve]
			);
			++index;
		}
		logger() << F("loadZones Completed") << L_endl;

		index = 0;
		for (Answer_R<R_TowelRail> towelRail : queries.q_towelRails) {
			towelRailArr[index].initialise(towelRail.id()
				, tempSensorArr[towelRail.rec().callTempSens]
				, relayArr[towelRail.rec().callRelay]
				, towelRail.rec().minutes_on
				, towelRail.rec().onTemp
				, *this
				, mixValveControllerArr[towelRail.rec().mixValve]
			);
			++index;
		}
		logger() << F("loadTowelRails Completed") << L_endl;
	}

	void TemperatureController::checkAndAdjust() { // Called every Arduino loop, enter once per second
		static uint8_t lastSeconds = clock_().seconds()-1;
		static uint8_t lastMins = clock_().minUnits()-1;
		bool isNewSecond = clock_().isNewSecond(lastSeconds);
		if (!isNewSecond) { return; } // Wait for next second.
		bool newMinute = clock_().isNewMinute(lastMins);
		bool checkPreHeat = clock_().minUnits() == 0; // each 10 minutes

		logger() << L_flush;
		zTempLogger() << L_flush;
		profileLogger() << L_flush;
		//logger() << L_time << "TC::checkAndAdjust" << (checkPreHeat ? " with Preheat" : " without Preheat") << L_endl;
		//logger() << L_time << "Check TS's" << L_endl;
		for (auto & ts : tempSensorArr) {
			ts.readTemperature();
			//logger() << F("TS:device 0x") << L_hex << ts.getAddress() << F_COLON << L_dec << ts.get_temp() << F(" Error? ") << ts.hasError() << L_endl;
			ui_yield();
		}

		if (newMinute) {
			//logger() << L_time << "Check BB & Zones" << L_endl;
			backBoiler.check();
			checkZones(checkPreHeat);
			if (checkPreHeat) {
				mixValveControllerArr[0].i2C().nextWireMode();
			}
		}

		//logger() << L_time << "Check MixV's" << L_endl;
		for (auto & mixValveControl : mixValveControllerArr) {
			mixValveControl.check();
			if (newMinute && checkPreHeat) mixValveControl.logMixValveOperation(true);
			if (mixValveControl.isUnrecoverable()) HardReset::arduinoReset("MixValveController"); 
			ui_yield(); 
		}

		//logger() << L_time << "Check TRd's" << L_endl;
		for (auto & towelRail : towelRailArr) {
			towelRail.check();
			ui_yield(); 
		}

		for ( auto & relay : relayArr) {
			relay.checkControllerStateCorrect();
		}
		relayController().updateRelays();
		if (static_cast<RelaysPort&>(relayController()).isUnrecoverable()) HardReset::arduinoReset("RelayController");
		//logger() << L_time << F("RelaysPort::updateRelays done") << L_endl;
	}

	void TemperatureController::checkZones(bool checkForPreHeat) {

		for (auto & zone : zoneArr) { 
			if (checkForPreHeat) zone.preHeatForNextTT();
			zone.setFlowTemp();
			ui_yield();
		}
	}
}