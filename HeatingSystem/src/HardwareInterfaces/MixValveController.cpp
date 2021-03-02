#include "MixValveController.h"
#include "A__Constants.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include <I2C_Talk_ErrorCodes.h>
#include <Logging.h>
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include <Timer_mS_uS.h>

void ui_yield();
Logger& profileLogger();

namespace HardwareInterfaces {
	using namespace Assembly;
	using namespace I2C_Talk_ErrorCodes;
//#if defined (ZPSIM)
//	using namespace std;
//	ofstream MixValveController::lf("LogFile.csv");
//
//#endif
	int MixValveController::wireMode = 2;

	void MixValveController::initialise(int index, int addr, UI_Bitwise_Relay * relayArr, UI_TempSensor * tempSensorArr, int flowTempSens, int storeTempSens) {
		//profileLogger() << F("MixValveController::ini ") << index << L_endl;
		setAddress(addr);
		_index = index;
		_relayArr = relayArr;
		_tempSensorArr = tempSensorArr;
		_flowTempSens = flowTempSens;
		_storeTempSens = storeTempSens;
		_controlZoneRelay = _index == M_DownStrs ? R_DnSt : R_UpSt;
		i2C().extendTimeouts(15000, 6, 1000);
		i2C().begin();
		//profileLogger() << F("MixValveController::ini done") << L_endl;
	}

	void MixValveController::waitForWarmUp() {
		if (*_timeOfReset_mS != 0) {
			auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
			while (!timeOut) {
				//logger() << L_time << F("MixV Warmup used: ") << timeOut.timeUsed() << L_endl;
				ui_yield();
			}
			*_timeOfReset_mS = 0;
		}
	}

	Error_codes MixValveController::testDevice() { // non-recovery test
		if (runSpeed() > 100000) set_runSpeed(100000);
		Error_codes status = _OK;
		uint8_t val1[1] = { 48 };
		uint8_t val2[1] = { 50 };
		waitForWarmUp();
		//status = I_I2Cdevice::write(Mix_Valve::flow_temp + _index * 16, 1, val1); // non-recovery write request temp
		//status |= I_I2Cdevice::write(Mix_Valve::request_temp + _index * 16, 1, val2); // non-recovery write request temp
		status |= I_I2Cdevice::read(Mix_Valve::mode + _index * 16, 1, val1); // non-recovery 
		status |= I_I2Cdevice::read(Mix_Valve::state + _index * 16, 1, val2); // non-recovery 
		//if (status || (val1[0] == 0 && val2[0] == 0)) return _I2C_ReadDataWrong;
		//status = I_I2Cdevice::write_verify(Mix_Valve::flow_temp + _index * 16, 1, val1); // non-recovery write request temp
		//status |= I_I2Cdevice::write_verify(Mix_Valve::request_temp + _index * 16, 1, (uint8_t*)&_mixCallTemp); // non-recovery write request temp

		return status;
	}

	uint8_t MixValveController::sendSetup() {
		uint8_t errCode;
		check();
		errCode = writeToValve(Mix_Valve::temp_i2c_addr, _tempSensorArr[_flowTempSens].getAddress());
		errCode |= writeToValve(Mix_Valve::max_ontime, VALVE_FULL_TRANSIT_TIME);
		errCode |= writeToValve(Mix_Valve::wait_time, VALVE_WAIT_TIME);
		logger() <<  F("MixValveController::sendSetup()") << i2C().getStatusMsg(errCode) << L_endl;
		return errCode;
	}

	uint8_t MixValveController::flowTemp() const { return _tempSensorArr[_flowTempSens].get_temp(); }

	bool MixValveController::check() { // called once per second
		sendFlowTemp();
		logMixValveOperation(false);
		return true;
	}

	void MixValveController::monitorMode() {
		_tempSensorArr[_flowTempSens].readTemperature();
		_relayArr[_controlZoneRelay].set(); // turn call relay ON
		relayController().updateRelays();
		sendFlowTemp();
		logMixValveOperation(true);
		//writeToValve(Mix_Valve::request_temp, _mixCallTemp);
	}

	void MixValveController::logMixValveOperation(bool log) {
		int8_t algorithmMode = (int8_t)readFromValve(Mix_Valve::mode); // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq
		bool ignoreThis = algorithmMode == Mix_Valve::e_Checking
			|| algorithmMode == Mix_Valve::e_AtLimit;

		if (log || !ignoreThis || valveStatus.algorithmMode != algorithmMode) {
			valveStatus.motorActivity = (int8_t)readFromValve(Mix_Valve::state); // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
			valveStatus.algorithmMode = algorithmMode;
			valveStatus.onTime = (uint8_t)readFromValve(Mix_Valve::count);
			valveStatus.valvePos = readFromValve(Mix_Valve::valve_pos);
			valveStatus.ratio = (uint8_t)readFromValve(Mix_Valve::ratio);
			profileLogger() << L_time << L_tabs << (_index == M_DownStrs ? "DS_Mix R/Is" : "US_Mix R/Is") << _mixCallTemp << flowTemp()  << showState() << valveStatus.onTime << valveStatus.valvePos << valveStatus.ratio << L_endl;
		}
	}

	const __FlashStringHelper* MixValveController::showState() {
			switch (valveStatus.algorithmMode) { // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat
			case Mix_Valve::e_Wait: return F("Wt");
			case Mix_Valve::e_Checking: return F("Chk");
			case Mix_Valve::e_Mutex: return F("Mx");
			case Mix_Valve::e_NewReq: return F("Req");
			case Mix_Valve::e_AtLimit: return F("Lim");
			case Mix_Valve::e_DontWantHeat: return F("Min");
			case Mix_Valve::e_Moving:
				switch (valveStatus.motorActivity) { // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
				case Mix_Valve::e_Moving_Coolest: return F("Min");
				case Mix_Valve::e_Cooling: return F("Cl");
				case Mix_Valve::e_Heating: return F("Ht");
				default: return F("MEr"); // e_Moving when e_Stop!
				}
			default: return F("SEr");
		}
	}

	void MixValveController::sendFlowTemp() {
		auto flowTemp = _tempSensorArr[_flowTempSens].get_temp();
		auto tempError = _tempSensorArr[_flowTempSens].hasError();
		if (!tempError) writeToValve(Mix_Valve::flow_temp, flowTemp);
		writeToValve(Mix_Valve::request_temp, _mixCallTemp);
	}

	void MixValveController::sendRequestFlowTemp(uint8_t callTemp) {
		//writeToValve(Mix_Valve::request_temp, callTemp); // done by sendFlowTemp()
		_mixCallTemp = callTemp;
	}

	bool MixValveController::amControlZone(uint8_t newCallTemp, uint8_t maxTemp, uint8_t zoneRelayID) { // highest callflow temp
		// Lambdas
		auto relayName = [](int id) -> const char* { // only for error reporting
			switch (id) {
			case R_Flat: return "Flat-UFH";
			case R_FlTR: return "Fl-TR";
			case R_HsTR: return "House-TR";
			case R_UpSt: return "US-UFH";
			case R_DnSt: return "DS-UFH";
			default: return "";
			}
		};

		auto setNewControlZone = [maxTemp, this, zoneRelayID, relayName]() {
			writeToValve(Mix_Valve::max_flow_temp, maxTemp);
			profileLogger() << L_time << F("MixValveController: ") << relayName(zoneRelayID) << F(" is new ControlZone.\t New MaxTemp: ") << maxTemp << L_endl;
			_limitTemp = maxTemp;
			_controlZoneRelay = zoneRelayID;
		};

		auto hasLimitPriority = [maxTemp, this, newCallTemp]() -> bool {
			auto zoneIsHeating = newCallTemp > MIN_FLOW_TEMP;
			if (zoneIsHeating && maxTemp <= _limitTemp) {
				_limitTemp = maxTemp;
				if (_mixCallTemp > _limitTemp) _mixCallTemp = 0; // trigger take control
				return true;
			} else return false;
		};
		
		auto amControllingZone = [this,zoneRelayID]() -> bool {return _controlZoneRelay == zoneRelayID && _limitTemp < 100; };

		auto checkForNewControllingZone = [newCallTemp, this, amControllingZone, hasLimitPriority, setNewControlZone]() {
			// controlZone is one calling for heat with lowest max-temp and highest call-temp.
			if (!amControllingZone() && hasLimitPriority() && newCallTemp > _mixCallTemp) { // new control zone
				setNewControlZone();
			}
		};
				
		// Algorithm
		checkForNewControllingZone();
		bool amInControl = amControllingZone();
		if (amInControl) {
			auto mv_FlowTemp = flowTemp();
#if defined (ZPSIM)
			bool debug;
			if (zoneRelayID == 3)
				debug = true;
			switch (_index) {
			case M_UpStrs: {// upstairs
				debug = true;
				if (mv_FlowTemp >= _mixCallTemp)
					bool debug = true;
				break; }
			case M_DownStrs: // downstairs
				debug = true;
				break;
			}
#endif
			
			if (newCallTemp <= MIN_FLOW_TEMP) {
				profileLogger() << L_time << F("MixValveController: ") << relayName(zoneRelayID) << F(" Released Control.") << L_endl;
				newCallTemp = MIN_FLOW_TEMP;
				_relayArr[_controlZoneRelay].clear(); // turn call relay OFF
				_limitTemp = 100; // release control of limit temp.
				amInControl = false;
			}
			else {
				_relayArr[_controlZoneRelay].set(); // turn call relay ON
				if (newCallTemp > _limitTemp) newCallTemp = _limitTemp;
			}

			//if (_mixCallTemp != newCallTemp) {
				sendRequestFlowTemp(newCallTemp);
			//} 
		}
		return amInControl;
	}


	// Private Methods

	int8_t MixValveController::relayInControl() const {
		return _relayArr[_controlZoneRelay].logicalState() == 0 ? -1 : _controlZoneRelay;
	}

	bool MixValveController::needHeat(bool isHeating) {
		if (relayInControl() == -1) return false;

		int storeTempAtMixer = _tempSensorArr[_storeTempSens].get_temp(); 
		auto minStoreTemp = isHeating ? _mixCallTemp + THERM_STORE_HYSTERESIS : _mixCallTemp;
		if (storeTempAtMixer < minStoreTemp /*|| readFromValve(Mix_Valve::status) == Mix_Valve::e_Water_too_cool*/) {
			profileLogger() << L_time << (index() == 0 ? "DS" : "US") << " Mix-Store Temp: " << storeTempAtMixer << L_endl;
			return true;
		}
		return false;
	}

	Error_codes  MixValveController::writeToValve(Mix_Valve::Registers reg, uint8_t value) {
		// All writable registers are single-byte
		auto status = reEnable(); // see if is disabled.
		waitForWarmUp();
		uint8_t mixReg = reg + _index * 16;
		if (status == _OK) {
			//status = write_verify(mixReg, 1, &value); // recovery-write
			status = write(mixReg, 1, &value); // recovery-write
			if (status) {
				logger() << L_time << F("MixValve Write device 0x") << L_hex << getAddress() << F(" Reg: ") << reg << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			} //else logger() << L_time << F("MixValve Write OK device 0x") << L_hex << getAddress() << L_endl;
		} else logger() << L_time << F("MixValve Write device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
		return status;
	}

	int16_t MixValveController::readFromValve(Mix_Valve::Registers reg) {
		int16_t value = 0;
		auto status = reEnable(); // see if is disabled
		waitForWarmUp();
		constexpr int CONSECUTIVE_COUNT = 1;
		constexpr int MAX_TRIES = 1;
		if (status == _OK) {
			uint8_t mixReg = reg + _index * 16;
			read(mixReg, 2, (uint8_t*)&value); // recovery
			status = read_verify_2bytes(mixReg, value, CONSECUTIVE_COUNT, MAX_TRIES); // Non-Recovery

			if (status) {
				logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << F(" Reg: ") << reg << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			}
		} else logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
		return value;
	}

	void MixValveController::setWireMode(int newMode) {
		wireMode = newMode;
		bool onAddrErr = wireMode & 0x01;
		bool onDataErr = wireMode & 0x02;
		bool alwaysStop = wireMode & 0x04;
		i2C().setEndAbort(onAddrErr, onDataErr, alwaysStop);
		logger() << "Wire Aborts on";
		if (onAddrErr) logger() << " AddrEr,";
		if (onDataErr) logger() << " DataEr,";
		if (alwaysStop) logger() << " AlwaysSendStop";
		logger() << L_endl;
	}
}