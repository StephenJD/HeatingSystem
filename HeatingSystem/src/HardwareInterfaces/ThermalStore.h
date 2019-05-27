#pragma once
#include "Arduino.h"
#include <RDB/src/RDB.h>
#include <HeatingSystem/src/LCD_UI/I_Record_Interface.h>
#include <HeatingSystem/src/Client_DataStructures/Data_ThermalStore.h>
#include "Temp_Sensor.h"
#include "..\Assembly\HeatingSystemEnums.h"

namespace HardwareInterfaces {
	class BackBoiler;
	class MixValveController;

	class ThermalStore : public LCD_UI::VolatileData {
	public:
		ThermalStore(I2C_Temp_Sensor * tempSensorArr, MixValveController (& mixValveControllerArr)[Assembly::NO_OF_MIX_VALVES], BackBoiler & backBoiler);
		//int16_t getFractionalOutsideTemp() const;
		uint8_t getTopTemp() const;
		//uint8_t getOutsideTemp() const;
		//bool storeIsUpToTemp() const; // just returns status flag
		bool dumpHeat() const;
		uint8_t currDeliverTemp() const { return _currDeliverTemp; }
		uint8_t calcCurrDeliverTemp(int callTemp) const;

		// Modifier
		void initialise(client_data_structures::R_ThermalStore thermStoreData);
		//bool check(); // checks zones, twl rads & thm store for heat requirement and sets relays accordingly
		void setLowestCWtemp(bool isFlowing);
		//void adjustHeatRate(byte change);
		bool needHeat(int currRequest, int nextRequest);
		uint8_t getGroundTemp() const;
		void calcCapacities();

	private:
		I2C_Temp_Sensor * _tempSensorArr;
		MixValveController(& _mixValveControllerArr)[Assembly::NO_OF_MIX_VALVES];
		BackBoiler & _backBoiler;
		client_data_structures::R_ThermalStore _thermStoreData;
		bool dhwDeliveryOK(int currRequest) const;
		bool dhwNeedsHeat(int callTemp, int nextRequest);

		uint8_t _currDeliverTemp = 45;
		bool _isHeating = false;

		float _upperC, _midC, _bottomC; // calculated capacity factors
		float _upperV, _midV, _bottomV; // calculated Volumes
		uint8_t _groundT = 15;
	};

}