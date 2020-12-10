#pragma once
#include "..\HardwareInterfaces\ThermalStore.h"
#include "..\HardwareInterfaces\BackBoiler.h"
#include "..\HardwareInterfaces\Zone.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "..\HardwareInterfaces\MixValveController.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_TowelRail.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include <RDB.h>

namespace Assembly {
	class Sequencer;

	class TemperatureController
	{
	public:
		TemperatureController(I2C_Recovery::I2C_Recover & recovery, Sequencer & sequencer, unsigned long * timeOfReset_mS);
		// Queries
		int outsideTemp() const { return thermalStore.getOutsideTemp(); }

		// Modifiers
		void checkAndAdjust();
		void checkZones();
		
		HardwareInterfaces::UI_TempSensor tempSensorArr[Assembly::NO_OF_TEMP_SENSORS];
		HardwareInterfaces::TowelRail towelRailArr[Assembly::NO_OF_TOWELRAILS];
		HardwareInterfaces::BackBoiler backBoiler;
		HardwareInterfaces::ThermalStore thermalStore;
		HardwareInterfaces::MixValveController mixValveControllerArr[NO_OF_MIX_VALVES];
		HardwareInterfaces::UI_Bitwise_Relay relayArr[Assembly::NO_OF_RELAYS];
		HardwareInterfaces::Zone zoneArr[NO_OF_ZONES];
	};

}
