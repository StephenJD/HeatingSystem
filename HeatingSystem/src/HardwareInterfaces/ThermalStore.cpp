#include "ThermalStore.h"
#include <Logging.h>
#include "BackBoiler.h"
#include "MixValveController.h"
#include "A__Constants.h"

namespace HardwareInterfaces {

	ThermalStore::ThermalStore(I2C_Temp_Sensor * tempSensorArr, MixValveController(&mixValveControllerArr)[Assembly::NO_OF_MIX_VALVES], BackBoiler & backBoiler)
		: _tempSensorArr(tempSensorArr)
		, _mixValveControllerArr(mixValveControllerArr)
		, _backBoiler(backBoiler)
	{}

	void ThermalStore::initialise(client_data_structures::R_ThermalStore thermStoreData) { 
		_thermStoreData = thermStoreData;
		calcCapacities();
	}

	//int16_t ThermalStore::getFractionalOutsideTemp() const {
	//	return f->tempSensorR(getVal(OutsideTS)).getFractionalSensTemp();
	//}

	uint8_t ThermalStore::getTopTemp() const {
		uint8_t temp = _tempSensorArr[_thermStoreData.OvrHeatTS].get_temp();
		if (I2C_Temp_Sensor::hasError()) temp = _tempSensorArr[_thermStoreData.DHWpreMixTS].get_temp();
		return temp;
	}

	//uint8_t ThermalStore::getOutsideTemp() const {
	//	return f->tempSensorR(getVal(OutsideTS)).getSensTemp();
	//}

	void ThermalStore::setLowestCWtemp(bool isFlowing) {
		if (isFlowing || (_groundT > getGroundTemp())) _groundT = getGroundTemp();
	}

	uint8_t ThermalStore::getGroundTemp() const {
		return  _tempSensorArr[_thermStoreData.GroundTS].get_temp();
	}

	uint8_t ThermalStore::getOutsideTemp() const {
		return  _tempSensorArr[_thermStoreData.OutsideTS].get_temp();
	}

	bool ThermalStore::dumpHeat() const {
		return getTopTemp() >= _thermStoreData.OvrHeatTemp;
	}

	//bool ThermalStore::check() { // checks zones, twl rads & thm store for heat requirement and sets relays accordingly
	//	--Zone_Run::z_period;
	//	uint8_t i;
	//	for (i = 0; i < f->towelRads.count(); ++i) { // takes 200mS, required every second
	//		f->towelRads[i].run().check(); // setFlowTemp, mixCall & switches relays
	//	}
	//
	//	for (i = 0; i < f->zones.count(); ++i) { // takes 850mS
	//		// DHW zone checks Mix Valves are hot enough & DHW is OK.
	//		f->zoneR(i).check(); // setZFlowTemp, mixCall & switches relays
	//	}
	//
	//	if (Zone_Run::z_period <= 0) Zone_Run::z_period = AVERAGING_PERIOD;
	//
	//	if (!temp_sense_hasError) {
	//		for (i = 0; i < f->mixingValves.count(); ++i) { // takes 104mS
	//			f->mixValveR(i).check(); // Check Mixing Valve flow temps
	//		}
	//	}
	//	return true;
	//}

	bool ThermalStore::needHeat(int currRequest, int nextRequest) {
		// called every Minute.
		setLowestCWtemp(false);
		bool needHeat = dhwNeedsHeat(currRequest, nextRequest);
		logger() << L_time << "ThermalStore:: currRequest: " << currRequest << " CurrDHW temp: " << _theoreticalDeliveryTemp << (needHeat ? " Asking for heat." : " Not asking for heat.") <<  (_isHeating ? " Is heating" : " Not heating")  << L_endl;
		if (!_isHeating && needHeat) logger() << L_time << "DHW Needs Heat. CurrRequest: " << currRequest << " Curr DHW temp: " << _theoreticalDeliveryTemp << L_endl;
		// Check temp for each mix valve		
		auto mixNeedsHeat = false;
		for (auto & mixV : _mixValveControllerArr) {
			mixNeedsHeat = mixNeedsHeat || mixV.needHeat(_isHeating);
		}
		if (/*!_isHeating && */mixNeedsHeat) logger() << L_time << "MixValve Needs Heat\n";
		needHeat |= mixNeedsHeat;
		if (_isHeating && !needHeat) {
			//f->eventS().newEvent(EVT_GAS_TEMP, f->tempSensorR(getVal(GasTS)).getSensTemp());
			logger() << "\tGas Turned OFF   Gas Flow Temp: " << _tempSensorArr[_thermStoreData.GasTS].get_temp();
			logger() << "\n\tCurr DHW temp: " << _theoreticalDeliveryTemp << L_endl;
		}
		else if (!_isHeating && needHeat) {
			logger() << "\tGas Turned ON\n";
			logger() << L_tabs << "GroundT:" << _groundT
				<< "Top:" << getTopTemp()
				<< "Mid:" << _tempSensorArr[_thermStoreData.MidDhwTS].get_temp()
				<< "Lower" << _tempSensorArr[_thermStoreData.LowerDhwTS].get_temp() << L_endl;
		}
		_isHeating = needHeat;
		return needHeat;
	}

	// Private methods

	void ThermalStore::calcCapacities() {
		// Using conductionConstant, flowRate & coilFraction calculate exponent k at each level
		// Actual Data without gas-reheat on shallow bath: Top:-3C, Mid:-8C, Bot:-4C.
		uint8_t flowRate = _thermStoreData.DHWflowRate;

		float cond = _thermStoreData.Conductivity; //25; //
		float F[4]; // First calc length of Heat Exchanger at level of sensor
		F[3] = ((float)_thermStoreData.TopSensHeight - _thermStoreData.LowerSensHeight); // Length of heat exchanger
		F[0] = ((float)_thermStoreData.MidSensHeight - _thermStoreData.LowerSensHeight) / 2.0F; // cm's
		F[1] = F[3] / 2.0F;
		F[2] = (_thermStoreData.TopSensHeight - _thermStoreData.MidSensHeight) / 2.0F;

		_bottomV = F[0] * pow((float)_thermStoreData.CylDia, 2) * 0.00071F; // vol in litres, allowing 10% for coil volume
		_midV = F[1] * pow((float)_thermStoreData.CylDia, 2) * 0.00071F; // vol in litres
		_upperV = (_thermStoreData.CylHeight - (_thermStoreData.TopSensHeight + _thermStoreData.MidSensHeight) / 2) * pow((float)_thermStoreData.CylDia, 2) * 0.00071F; // vol in litres
		// Now calc Fraction of heatexchanger at each level
		F[0] = F[0] / F[3];
		F[1] = F[1] / F[3];
		F[2] = F[2] / F[3];
		float k[3];
		k[0] = (0.046F - 0.0031F*cond)*F[0] + (.026F * cond - .032F)*F[0] / pow(float(flowRate / 60.0), float(1.0 - .001*cond + (.003*cond - .025)*F[0]));
		k[1] = (0.046F - 0.0031F*cond)*F[1] + (.026F * cond - .032F)*F[1] / pow(float(flowRate / 60.0), float(1.0 - .001*cond + (.003*cond - .025)*F[1]));
		k[2] = (0.046F - 0.0031F*cond)*F[2] + (.026F * cond - .032F)*F[2] / pow(float(flowRate / 60.0), float(1.0 - .001*cond + (.003*cond - .025)*F[2]));

		_bottomC = 1.0F - exp(-k[0]);
		_midC = 1.0F - exp(-k[1]);
		_upperC = 1.0F - exp(-k[2]);
		logger() << "\nThermalStore::calcCapacities\t_upperC " << _upperC << " _midC " << _midC << " _bottomC " << _bottomC << L_endl;
	}

	uint8_t ThermalStore::calcCurrDeliverTemp(int callTemp) const {
		float boilerPwr = _thermStoreData.BoilerPower;
		uint8_t duration = _thermStoreData.DHWflowTime;

		float topT = getTopTemp();
		float midT = _tempSensorArr[_thermStoreData.MidDhwTS].get_temp();
		float botT = _tempSensorArr[_thermStoreData.LowerDhwTS].get_temp();
		//logger() << "\nThermalStore::calcCurrDeliverTemp\t_topT " << topT << " mid " << midT << " Bot " << botT << L_endl;

		// using Capacities, ground and store temps, calc HW temp at each level
		float HWtemp[3];
		HWtemp[0] = _groundT + (botT - _groundT) * _bottomC;
		HWtemp[1] = HWtemp[0] + (midT - HWtemp[0]) * _midC;
		HWtemp[2] = HWtemp[1] + (topT - HWtemp[1]) * _upperC;
		//logger() << "\t_HWtemp[0] " << HWtemp[0] << " HWtemp[1] " << HWtemp[1] << " HWtemp[2] " << HWtemp[2] << L_endl;
		// Using HWtemps, calculate share of energy for each section of store
		float share[3];
		share[0] = (HWtemp[0] - _groundT) / (HWtemp[2] - _groundT);
		share[1] = (HWtemp[1] - HWtemp[0]) / (HWtemp[2] - _groundT);
		share[2] = (HWtemp[2] - HWtemp[1]) / (HWtemp[2] - _groundT);
		// Calc final store temps using ratio of DHW vol to storeVol
		float storeTemps[3];
		float factor = ((callTemp - _groundT) * _thermStoreData.DHWflowRate * .07F - boilerPwr) * duration *1.43F;
		storeTemps[0] = botT - factor * share[0] / _bottomV;
		storeTemps[1] = midT - factor * share[1] / _midV;
		storeTemps[2] = topT - factor * share[2] / _upperV;
		//logger() << "\t_storeTemps[0] " << storeTemps[0] << " storeTemps[1] " << storeTemps[1] << " storeTemps[2] " << storeTemps[2] << L_endl;

		// Calc final HW temps
		HWtemp[0] = _groundT + (storeTemps[0] - _groundT) * _bottomC;
		HWtemp[1] = HWtemp[0] + (storeTemps[1] - HWtemp[0]) * _midC;
		HWtemp[2] = HWtemp[1] + (storeTemps[2] - HWtemp[1]) * _upperC + 0.5f;
		return (uint8_t)HWtemp[2];
	}

	bool ThermalStore::dhwNeedsHeat(int callTemp, int nextRequest) {
		_theoreticalDeliveryTemp = calcCurrDeliverTemp(nextRequest > callTemp ? nextRequest : callTemp);
		//logger() << "\nThermalStore::dhwNeedsHeat\t_theoreticalDeliveryTemp " << _theoreticalDeliveryTemp << L_endl;
		// Note that because callTemp determins the rate of heat-extraction, currDeliverTemp, which is temperature AFTER filling a bath, will change if callTemp changes.
		static bool hasRequestedCondReduction = false;
		bool dhwTempOK = dhwDeliveryOK(callTemp);
		if (!hasRequestedCondReduction && _theoreticalDeliveryTemp >= callTemp && !dhwTempOK) { // reduce conductivity if claims to be hot enought, but isn't
			hasRequestedCondReduction = true;
			//f->eventS().newEvent(EVT_THS_COND_CHANGE,S1_byte(getVal(Conductivity)) - 1);
			logger() << "\nThermalStore::dhwNeedsHeat\tPre-Mix temp too low - reduce cond?\t " 
				<< " Cond: " << _thermStoreData.Conductivity
				<< " CallTemp: " << callTemp
				<< " NextCallTemp: " << nextRequest
				<< " Calculated DHW: " << _theoreticalDeliveryTemp
				<< " DHW-flowTemp: " << _tempSensorArr[_thermStoreData.DHWFlowTS].get_temp()
				<< " DHW-preMixTemp: " << L_fixed << _tempSensorArr[_thermStoreData.DHWpreMixTS].get_fractional_temp() / 256 << L_endl;
			//setVal(Conductivity, uint8_t(getVal(Conductivity)) - 1);
		}
		if (dhwTempOK) hasRequestedCondReduction = false;
		uint8_t addHysteresis = 0;
		if (!_backBoiler.check() && _isHeating) { // only add hysteresis if stove not on.
			addHysteresis = THERM_STORE_HYSTERESIS;
		}
		return _theoreticalDeliveryTemp < (callTemp + addHysteresis);	// If HWtemp[2] is not hot enough, we need heat.
	}

	bool ThermalStore::dhwDeliveryOK(int currRequest) const {
		/*
		Compare DHW temp each side of thermostatic valve to see if cold is being mixed in.
		If temps are the same, tank is too cool.
		*/
		bool tempError = false;
		auto dhwFlowTemp = _tempSensorArr[_thermStoreData.DHWFlowTS].get_temp();
		auto DHWpreMixTemp = _tempSensorArr[_thermStoreData.DHWpreMixTS].get_temp();
		if (DHWpreMixTemp < 0 || dhwFlowTemp > DHWpreMixTemp + 2) tempError = true;
		return (tempError || dhwFlowTemp >= currRequest || DHWpreMixTemp > dhwFlowTemp);
	}
}