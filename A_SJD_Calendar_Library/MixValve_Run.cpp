#include "debgSwitch.h"
#include "MixValve_Run.h"
#include "D_Factory.h"
#include "TempSens_Run.h"
#include "Relay_Run.h"
#include "Zone_Run.h"
#include "I2C_Helper.h" // for serial print relay status
#if defined (ZPSIM)
	#include <ostream>
	using namespace std;	
	ofstream MixValve_Run::lf("LogFile.csv");

#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//enum mixValveVals {mvStoreSensor,MV_COUNT};
	//extern MultiCrystal lcd;
extern unsigned long timeOfReset_mS;
U1_byte speedTestDevice(int addr);

MixValve_Run::MixValve_Run() : 
	_mixCallTemp(MIN_FLOW_TEMP),
	_controlZoneRelay(0),
	_limitTemp(60)
	{
	#if defined (ZPSIM)
	 motorState = 0; // required by simulator
	#endif
	}

U1_byte MixValve_Run::getStoreTemp() const {
	return f->tempSensorR(getVal(mvStoreSensor)).getSensTemp();
}

#if defined (ZPSIM)
void MixValve_Run::reportMixStatus() const {
	//lf << "Control Relay:\t" << (int)_controlZoneRelay << "\tCallT:\t" << (int)_mixCallTemp;
	//lf << "\tFlowT:\t" << (int)f->tempSensorR(_controlFlowSensor).getSensTemp();
	//lf << "\tValvePos:\t" << (int)_valvePos << "\tOn_Time:\t" << (int)_onTime << "\tOnTime_Ratio\t" << (int)_onTimeRatio;
	//lf << "\tState:\t" << (int)_state << "\tMotor:\t" << (int)motorState << endl;
}
#endif

U1_byte MixValve_Run::sendSetup() const {
	U1_byte errCode;
	errCode = writeToValve(Mix_Valve::flow_temp, f->tempSensorR(getVal(mvFlowSensID)).getSensTemp());
	errCode |= writeToValve(Mix_Valve::request_temp, _mixCallTemp);
	errCode |= writeToValve(Mix_Valve::ratio, 10);
	errCode |= writeToValve(Mix_Valve::temp_i2c_addr, f->tempSensors[getVal(mvFlowSensID)].getVal(0));
	errCode |= writeToValve(Mix_Valve::max_ontime, VALVE_FULL_TRANSIT_TIME);
	errCode |= writeToValve(Mix_Valve::wait_time, VALVE_WAIT_TIME);
	//errCode |= writeToValve(Mix_Valve::max_flow_temp, VALVE_WAIT_TIME);
	return errCode;
}

bool MixValve_Run::check() {
	writeToValve(Mix_Valve::flow_temp, f->tempSensorR(getVal(mvFlowSensID)).getSensTemp());
	return true;
}

const char * relayName (int id) { // only for error reporting in next function
	switch (id) {
	case r_Flat: return "Flat";
	case r_FlTR: return "Fl-TR";
	case r_HsTR: return "House-TR";
	case r_UpSt: return "US";
	case r_DnSt: return "DS";
	default: return "";
	}
}

bool MixValve_Run::amControlZone(U1_byte callTemp, U1_byte maxTemp, U1_byte relayID) { // highest callflow temp
	if (callTemp > MIN_FLOW_TEMP && maxTemp < _limitTemp) 
		_limitTemp = maxTemp;
	if (_controlZoneRelay == relayID || callTemp > _mixCallTemp) { 
		#if defined (ZPSIM)
			bool debug;
			if (relayID == 3)
				debug = true;
			U1_byte index = epD().index();
			switch (index) {
			case 0:{// upstairs
				debug = true;
				//if (f->tempSensorR(_controlFlowSensor).getSensTemp() >= _mixCallTemp)
				if (readFromValve(Mix_Valve::flow_temp) >= _mixCallTemp)
					bool debug = true;
				break;}
			case 1: // downstairs
				debug = true;
				break;
			}
		#endif
		if (_controlZoneRelay != relayID ) { // new control zone
			writeToValve(Mix_Valve::control,Mix_Valve::e_stop_and_wait); // trigger stop and wait on valve
			writeToValve(Mix_Valve::max_flow_temp, maxTemp);
			logToSD("MixValve_Run::amControlZone\t New CZ - Write new MaxTemp: ", maxTemp);
		}
		_controlZoneRelay = relayID;
		if (callTemp <= MIN_FLOW_TEMP) {
			callTemp = MIN_FLOW_TEMP;
			f->relayR(_controlZoneRelay).setRelay(0); // turn call relay OFF
			_limitTemp = 100; // reset since it might have been the limiting zone
		} else {
			f->relayR(_controlZoneRelay).setRelay(1); // turn call relay ON
			if (callTemp > _limitTemp) 
				callTemp = _limitTemp;
		}
		U1_byte mixValveCallTemp = readFromValve(Mix_Valve::request_temp);
		if (_mixCallTemp != callTemp || _mixCallTemp != mixValveCallTemp){
			_mixCallTemp = callTemp;
			logToSD("MixValve_Run::amControlZone MixID: ", epD().index());
			logToSD(relayName(relayID));
			logToSD("MixValve_Run::request_temp was: ", mixValveCallTemp);
			logToSD("MixValve_Run::writeToValve::request_temp", _mixCallTemp);
			logToSD("MixValve_Run::Actual flow_temp: ", readFromValve(Mix_Valve::flow_temp));
			writeToValve(Mix_Valve::request_temp, _mixCallTemp);
			writeToValve(Mix_Valve::control, Mix_Valve::e_new_temp);
		}
		return true;
	} else return false;
}


// Private Methods

bool MixValve_Run::controlZoneRelayIsOn() const {
	return (f->relayR(_controlZoneRelay).getRelayState() !=0);
}

bool MixValve_Run::needHeat(bool isHeating) const {
	if (!controlZoneRelayIsOn()) return false;
	else if (isHeating) {
		return (getStoreTemp() < _mixCallTemp + THERM_STORE_HYSTERESIS);
	} else {
		return (readFromValve(Mix_Valve::status) == Mix_Valve::e_Water_too_cool && getStoreTemp() < _mixCallTemp);
	}
}

U1_byte MixValve_Run::writeToValve(Mix_Valve::Registers reg, U1_byte value) const {
	unsigned long waitTime = 3000UL + timeOfReset_mS;
	int NO_OF_ATTEMPTS = 4;
	uint8_t hasFailed;
	int attempts = NO_OF_ATTEMPTS;
	do {
		if (attempts == NO_OF_ATTEMPTS / 2) {
			speedTestDevice(MIX_VALVE_I2C_ADDR);
		}
		hasFailed = i2C->write(MIX_VALVE_I2C_ADDR, reg + (epD().index() * 16), value);
	} while (hasFailed && (--attempts > 0 || millis() < waitTime));

	if (NO_OF_ATTEMPTS - attempts > 0) {
		if (attempts > 0) {
			logToSD("MixArduino write OK after" , NO_OF_ATTEMPTS - attempts + 1);
		} else {
			logToSD("MixArduino write Failed after" , NO_OF_ATTEMPTS - attempts);
		}
	}
	return hasFailed;
}

U1_byte MixValve_Run::readFromValve(Mix_Valve::Registers reg) const {
	unsigned long waitTime = 3000UL + timeOfReset_mS;
	int NO_OF_ATTEMPTS = 4;
	uint8_t hasFailed;
	int attempts = NO_OF_ATTEMPTS;
	U1_byte value;	
	do {
		if (attempts == NO_OF_ATTEMPTS / 2) {
			speedTestDevice(MIX_VALVE_I2C_ADDR);
		}
		hasFailed = i2C->read(MIX_VALVE_I2C_ADDR, reg + (epD().index() * 16), 1, &value);
	} while (hasFailed && (--attempts > 0 || millis() < waitTime));
	
	if (NO_OF_ATTEMPTS - attempts > 0) {
		if (attempts > 0) {
			logToSD("MixArduino read OK after" , NO_OF_ATTEMPTS - attempts + 1);
		} else {
			logToSD("MixArduino read Failed after" , NO_OF_ATTEMPTS - attempts);
		}
	}
	return value;
}
