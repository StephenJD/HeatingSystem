#pragma once
#include <Arduino.h>
#include <RDB.h>
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino
#include "..\Client_DataStructures\Data_ThermalStore.h" // relative path required by Arduino
#include "..\Assembly\HeatingSystemEnums.h" // relative path required by Arduino

namespace HardwareInterfaces {
	class BackBoiler;
	class MixValveController;
	class UI_TempSensor;

	class ThermalStore : public LCD_UI::VolatileData {
	public:
		ThermalStore(UI_TempSensor * tempSensorArr, MixValveController (& mixValveControllerArr)[Assembly::NO_OF_MIX_VALVES], BackBoiler & backBoiler);
		//int16_t getFractionalOutsideTemp() const;
		uint8_t getTopTemp() const;
		//uint8_t getOutsideTemp() const;
		//bool storeIsUpToTemp() const; // just returns status flag
		bool dumpHeat() const;
		uint8_t currDeliverTemp() const { return _theoreticalDeliveryTemp; }
		uint8_t calcCurrDeliverTemp(int callTemp, float groundT, float topT, float midT, float botT) const;
		uint8_t getGroundTemp() const;
		uint8_t getOutsideTemp() const;
		bool backBoilerIsHeating() const;

		// Modifier
		void initialise(client_data_structures::R_ThermalStore thermStoreData);
		//bool check(); // checks zones, twl rads & thm store for heat requirement and sets relays accordingly
		void setLowestCWtemp(bool isFlowing);
		//void adjustHeatRate(byte change);
		bool needHeat(int currRequest, int nextRequest);
		void calcCapacities();

	private:
		UI_TempSensor * _tempSensorArr;
		MixValveController(& _mixValveControllerArr)[Assembly::NO_OF_MIX_VALVES];
		BackBoiler & _backBoiler;
		client_data_structures::R_ThermalStore _thermStoreData;
		bool dhwDeliveryOK(int currRequest) const;
		bool dhwNeedsHeat(int callTemp, int nextRequest);

		uint8_t _theoreticalDeliveryTemp = 45;
		bool _isHeating = false;

		float _upperC, _midC, _bottomC; // calculated capacity factors
		float _upperV, _midV, _bottomV; // calculated Volumes
		uint8_t _groundT = 15;
		mutable bool _tempSensorError = false;
	};

}