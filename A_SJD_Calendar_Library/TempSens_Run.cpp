#include "debgSwitch.h"
#include "TempSens_Run.h"
#include "I2C_Helper.h"
#include "D_Factory.h"
#include "Event_Stream.h"

uint8_t resetI2C(I2C_Helper & i2c, int addr);

extern I2C_Helper_Auto_Speed<27> * i2C;

#if defined (ZPSIM)
	#if !defined (TEST_PROGSIM)
		#include "..\Graphs\ThermalMass.h"
		extern ThermalMass upStairs, downStairs, flat, us_wood_floor, flat_wood_floor, ds_conc_floor;
		extern ThermalMass usMixValve, dsMixValve;
	#endif
//									F_R, F_TrS,	H_TrS,	E_TrS,
uint8_t TempSens_Run::tempSens[] = {15,	37,		37,		37,
	18, 16, 6,	15,	  50,	45,	 40,	 35,  30,  50,	58,	  64,    80,   48};
//	18, 16, 6,	14,	  50,	45,	 40,	 35,  30,  52,	63,	  72,    80,   48};
 // U_R,D_R,OS, CWin, Pdhw, DHW, U_UF, D_UF, Sol, TkDs, TkUs, TkTop, GasF, MF_F
#endif

TempSens_Run::TempSens_Run() : lastGood(0), timeExpired(0) {}

S2_byte TempSens_Run::getFractionalSensTemp() const { // whole + fractional degrees
	if (epD().index() == 255) return 0; // why would this ever be???
	S2_byte fractionalTemp;
	U1_byte readFailed = 0;
	uint8_t temp[2];
	#if defined (ZPSIM)
		#if defined (TEMP_SIMULATION)
			switch (epD().index()) {
			case F_R: temp[0] = (uint8_t) flat.getTemp(); break;
			case U_R: temp[0] = (uint8_t) upStairs.getTemp(); break;
			case D_R: temp[0] = (uint8_t) downStairs.getTemp(); break;
			case U_UF: temp[0] = (uint8_t) usMixValve.getTemp(); break;
			case D_UF: temp[0] = (uint8_t) dsMixValve.getTemp(); break;
			default:
				temp[0] = tempSens[epD().index()];
			}
		#else
			temp[0] = tempSens[epD().index()];
		#endif
		temp[1] = 0;
		fractionalTemp = temp[0] * 256;
		readFailed = 0;
	#else
		if (millis() > timeExpired) { // only re-read if 1-Second valid temp time expired.
			int count = 2;	
			do {
				readFailed =  i2C->read(getVal(0), DS75LX_Temp, 2, temp);
				fractionalTemp = temp[0] * 256 + temp[1];
				if (readFailed || fractionalTemp != lastGood) { // confirm reading OK
					readFailed = i2C->read(getVal(0), DS75LX_HYST_REG, 2, temp);
					readFailed |= (temp[0] != 75);
					readFailed |= i2C->read(getVal(0), DS75LX_LIMIT_REG, 2, temp);
					readFailed |= (temp[0] != 80);
				}
				if (readFailed) resetI2C(*i2C, getVal(0));
			} while (readFailed != 0 && --count > 0);
			if (readFailed == 0) timeExpired = millis() + 1000;
		} else fractionalTemp = lastGood;
	#endif
	
	if (readFailed) {
		if (temp_sense_hasError == false) {
			//f->eventS().newEvent(ERR_TEMP_SENS_READ,epD().index());
			//logToSD("TempSens_Run::getFractionalSensTemp\tTempSens failure\tERR_TEMP_SENS_READ",epD().index(),epD().getName());
			temp_sense_hasError = true;
		}
		fractionalTemp = lastGood;
		//resetI2C(); // constant resets are not helpful!
	} else {
		temp_sense_hasError = false;
		lastGood = fractionalTemp;
	}
	return fractionalTemp;
}

S1_byte TempSens_Run::getSensTemp() const {
	S2_byte returnVal = getFractionalSensTemp();
	returnVal += 128; // round to nearest degree
	returnVal = returnVal / 256 ;
	return S1_byte(returnVal);
}

#if defined (ZPSIM)
void TempSens_Run::setTemp(S1_byte temp) {
	tempSens[epD().index()] = temp;
}

void TempSens_Run::changeTemp(S1_byte change) {
	tempSens[epD().index()] += change;
}
#endif