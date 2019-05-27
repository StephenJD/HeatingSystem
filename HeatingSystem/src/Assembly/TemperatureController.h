#pragma once
#include "..\HardwareInterfaces\ThermalStore.h"
#include "..\HardwareInterfaces\BackBoiler.h"
#include "..\HardwareInterfaces\Zone.h"
#include "..\HardwareInterfaces\Relay.h"
#include "..\HardwareInterfaces\MixValveController.h"
#include "..\HardwareInterfaces\Temp_Sensor.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include <RDB\src\RDB.h>

namespace Assembly {

	class TemperatureController
	{
	public:
		TemperatureController(I2C_Talk & i2c, RelationalDatabase::RDB<TB_NoOfTables> & db, unsigned long * timeOfReset_mS);
		
		// Modifiers
		void checkAndAdjust();


		
		HardwareInterfaces::I2C_Temp_Sensor tempSensorArr[Assembly::NO_OF_TEMP_SENSORS];
		HardwareInterfaces::BackBoiler backBoiler;
		HardwareInterfaces::ThermalStore thermalStore;
		HardwareInterfaces::RelaysPort relaysPort;
		HardwareInterfaces::MixValveController mixValveControllerArr[NO_OF_MIX_VALVES];
		HardwareInterfaces::Relay relayArr[Assembly::NO_OF_RELAYS];
		HardwareInterfaces::Zone zoneArr[NO_OF_ZONES];
	};

}
