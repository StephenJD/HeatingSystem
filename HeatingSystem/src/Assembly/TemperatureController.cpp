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
namespace arduino_logger {
	Logger& zTempLogger();
	Logger& profileLogger();
}

using namespace arduino_logger;
using namespace I2C_Talk_ErrorCodes;

namespace Assembly {
	using namespace RelationalDatabase;
	using namespace HardwareInterfaces;
	using namespace client_data_structures;
	using namespace I2C_Recovery;

	TemperatureController::TemperatureController(I2C_Recover & recovery, HeatingSystem_Queries& queries, Sequencer& sequencer, i2c_registers::I_Registers& prog_registers) :
		tempSensorArr{ recovery }
		, backBoiler(tempSensorArr[T_MfF], tempSensorArr[T_Sol], relayArr[R_MFS])
		, thermalStore(tempSensorArr, mixValveControllerArr, backBoiler)
		, mixValveControllerArr{ {recovery, prog_registers}, {recovery, prog_registers} }
		, _prog_registers(prog_registers)
	{
		Zone::setSequencer(sequencer);
		int index = 0;
		logger() << F("loadtempSensors...") << L_endl;

		for (Answer_R<R_TempSensor> tempSensor : queries.q_tempSensors) {
			tempSensorArr[index].initialise(tempSensor.id(), tempSensor.rec().address); // Reads temp
			++index;
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
				, mixValveControl.rec().flowTS_addr
				, tempSensorArr[mixValveControl.rec().storeTempSens]
			);
			++index;
		}
		logger() << F("load mixValveControllerArr Completed") << L_endl;
		
		index = 0;
		for (Answer_R<R_Zone> zone : queries.q_Zones) {
			auto remoteTS_register = (PROG_REG_RC_US_OFFSET + OLED_Thick_Display::R_ROOM_TEMP) + OLED_Thick_Display::R_DISPL_REG_SIZE * index;
			zoneArr[index].initialise(
				zone
				, *i2c_registers::RegAccess(_prog_registers).ptr(remoteTS_register)
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

	bool TemperatureController::isNewSecond() const {
		static uint8_t lastSeconds = clock_().seconds()-1;
		return clock_().isNewSecond(lastSeconds);
	}

	Status TemperatureController::checkAndAdjust() { // Called once per second
		auto status = ALL_OK;

		//logger() << L_time << "Check TS's" << L_endl;
		//if (!readTemperaturesOK()) status = TS_FAILED;

		auto mixV_OK = true;
		for (auto & mixValveControl : mixValveControllerArr) {
			//logger() << L_time << "Check mixValveControl" << L_endl;
			if (!mixValveControl.tuningMixV()) {
				mixV_OK &= mixValveControl.readReg_and_log();
			}
			//ui_yield(); 
		}
		if (!mixV_OK) status = MV_FAILED;

		//logger() << L_time << "Check TRd's" << L_endl;
		for (auto & towelRail : towelRailArr) {
			towelRail.check();
			//ui_yield(); 
		}

		for ( auto & relay : relayArr) {
			relay.checkControllerStateCorrect();
		}
		if (relayController().updateRelays() != _OK) status = RELAYS_FAILED;
		//logger() << L_time << F("RelaysPort::updateRelays done") << L_endl;
		for (auto& zone : zoneArr) {
			if (zone.getCallFlowT() > MIN_FLOW_TEMP && !zone.isCallingHeat()) {
				logger() << L_time << "Zone[" << zone.id() << "]\t should be calling but isn't" << L_endl;
				zone.setFlowTemp();
			}
		}
		return status;
	}

	void TemperatureController::checkZoneRequests(bool checkForPreHeat) { // must be called once every 10 mins
		for (auto& zone : zoneArr) { 
			if (checkForPreHeat) zone.preHeatForNextTT();
			zone.setFlowTemp();
		}
	}

	void TemperatureController::resetZones() {
		for (auto& zone : zoneArr) {
			zone.refreshProfile(true);
		}
	}

	bool TemperatureController::readTemperaturesOK() {
		auto ts_OK = true;
		for (auto& ts : tempSensorArr) {
			ts_OK &= (ts.readTemperature() == _OK);
			//ui_yield();
		}
		return ts_OK;
	}

}