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
		void initialise(client_data_structures::R_ThermalStore thermStoreData);

		// Queries
		uint8_t getTopTemp() const;
		bool dumpHeat() const;
		uint8_t currDeliverTemp() const { return _theoreticalDeliveryTemp; }
		uint8_t calcCurrDeliverTemp(int callTemp, float groundT, float topT, float midT, float botT) const;
		uint8_t getGroundTemp() const;
		int8_t getOutsideTemp() const;
		bool backBoilerIsHeating() const;
		int8_t tooCoolRequestOrigin() const { return _heatRequestSource; }
		int8_t heatingDemandFrom() const { return _heatingDemand; }
		const __FlashStringHelper* principalLoad() const;

		// Modifier
		void setLowestCWtemp(bool isFlowing);
		bool needHeat(int currRequest, int nextRequest);
		void calcCapacities();

	private:
		UI_TempSensor * _tempSensorArr;
		MixValveController(& _mixValveControllerArr)[Assembly::NO_OF_MIX_VALVES]; // ref to array of MixValveControllers
		BackBoiler & _backBoiler;
		client_data_structures::R_ThermalStore _thermStoreData;
		bool dhwDeliveryOK(int currRequest) const;
		bool dhwNeedsHeat(int callTemp, int nextRequest);

		uint8_t _theoreticalDeliveryTemp = 45;
		bool _isHeating = false;
		int8_t _heatRequestSource = -1; // only used for logging
		int8_t _heatingDemand = -1; // TempConst only calculated if there is no heating demand

		float _upperC, _midC, _bottomC; // calculated capacity factors
		float _upperV, _midV, _bottomV; // calculated Volumes
		uint8_t _groundT = 15;
		mutable bool _tempSensorError = false;
	};

}