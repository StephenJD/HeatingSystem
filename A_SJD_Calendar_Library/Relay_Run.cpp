#include "debgSwitch.h"
#include "Relay_Run.h"
#include "I2C_Helper.h"
#include "D_Factory.h"
#include "Event_Stream.h"
#include "TempSens_Run.h"
#include "C_Displays.h"

extern Remote_display * hall_display;
extern Remote_display * flat_display;
extern Remote_display * us_display;

#if defined (ZPSIM)
	#include "EEPROM_Utilities.h"
	using namespace EEPROM_Utilities;
#endif
#define I2C_DATA 20

// enum relayVals {rDeviceAddr,rPortNo, rActiveState};
I2C_Helper_Auto_Speed<27> * i2C(0);

modes Relay_Run::switch_mode = e_onChange_read_OUTafter;
bool Relay_Run::I2C_OK = false;
unsigned long timeOfReset_mS;
bool initialisationRequired = true;

U4_byte Relay_Run::I2C_freq = 10000; //I2C_Helper::MIN_I2C_FREQ;
U1_count Relay_Run::noOfErrors =0;
U1_count Relay_Run::rewriteSuccess =0;
U1_count Relay_Run::reduceSpeedSuccess =0;
U1_count Relay_Run::iniSuccess =0;
U1_count Relay_Run::resetSuccess =0;
U1_count Relay_Run::latch_port_disagree =0;

U1_byte Relay_Run::optRelayRegister = ~IO8_PORT_OptCouplActive; // copies of registers in ports
U1_byte Relay_Run::prevOptRelayRegister; // copies of registers in ports

uint8_t initialiseTempSensorsFailed() {
	// Set room-sensors to high-res	
	U1_byte thisFailed = i2C->write(0x70,DS75LX_Config,0x60);
	thisFailed |= i2C->write(0x36,DS75LX_Config,0x60);
	thisFailed |= i2C->write(0x74,DS75LX_Config,0x60);
	return thisFailed;
}

uint8_t resetI2C(I2C_Helper & i2c, int addr){
	const uint8_t NO_OF_TRIES = 2;
	static bool isInReset = false;
	static unsigned long nextAllowableResetTime = millis() + 600000ul; // resets every 10 mins for failed speed-test
	if (isInReset) return 0;
	bool thisAddressFailedSpeedTest = (i2c.getThisI2CFrequency(addr) == 0);
	if (thisAddressFailedSpeedTest && millis() < nextAllowableResetTime) return 0;
	isInReset = true;
	nextAllowableResetTime = millis() + 600000ul;
	uint8_t hasFailed = 0, iniFailed = 0, count = NO_OF_TRIES;
	I2C_Helper::TestFnPtr origFn = i2c.getTimeoutFn();
	i2c.setTimeoutFn(hardReset_Performed);

	do {
		logToSD("ResetI2C... for ", addr, "try:",NO_OF_TRIES-count+1);
		hardReset_Performed(i2c, addr);

		if (addr != 0) {
			if (getTestFunction(addr)(i2c, addr)) {
			logToSD(" Re-test Speed for", addr, " Started at: " , i2c.getI2CFrequency());
			hasFailed = speedTestDevice(addr);
			logToSD(" Re-test Speed Done at:", i2c.getThisI2CFrequency(addr), i2c.getError(hasFailed));
			} else logToSD(" Re-test was OK");
		}

		if (initialisationRequired) {
			iniFailed = initialiseRemoteDisplaysFailed();
			iniFailed = iniFailed | initialisePortsFailed(i2c, IO8_PORT_OptCoupl);
			iniFailed = iniFailed | initialiseTempSensorsFailed();
			initialisationRequired = (iniFailed == 0);
		}
	} while ((hasFailed || iniFailed) && --count > 0);

	i2c.setTimeoutFn(origFn);
	isInReset = false;
	return hasFailed;
}

I2C_Helper::TestFnPtr getTestFunction(int addr) {
	if (addr == 0x10) {
		return getMixArduinoFailed;
	} else if (addr == 0x20) {
		return initialisePortsFailed;
	} else if (addr >= 0x24 && addr <= 0x26) {
		return remoteDisplayFailed;
	} else {
		return tempSensorFailed;
	}
}

U1_byte speedTestDevice(int addr) {
	I2C_Helper::TestFnPtr origFn = i2C->getTimeoutFn();
	i2C->setTimeoutFn(hardReset_Performed);
	i2C->result.reset();
	i2C->result.foundDeviceAddr = addr;
	i2C->speedTestNextS(getTestFunction(addr));
	U1_byte failedTest = i2C->result.error;
	i2C->result.reset();
	i2C->setTimeoutFn(origFn);
	return failedTest;
}

bool i2C_is_released() {
  unsigned long downTime = micros() + 20L;
  while (digitalRead(I2C_DATA) == LOW && micros() < downTime);
  return (digitalRead(I2C_DATA) == HIGH);
}

U1_byte hardReset_Performed(I2C_Helper & i2c, int addr) {
	uint8_t resetPerformed = 0;

	if (!i2C_is_released()) {
		digitalWrite(RESET_LED_PIN_N, LOW);
		resetPerformed = 1;
		initialisationRequired = true;

		unsigned long downTime = 2000; // was 64000
		do {
			downTime *= 2;
			pinMode(abs(RESET_OUT_PIN), OUTPUT);
			digitalWrite(abs(RESET_OUT_PIN), (RESET_OUT_PIN < 0) ? LOW : HIGH);
			delayMicroseconds(downTime); // was 16000
			digitalWrite(abs(RESET_OUT_PIN), (RESET_OUT_PIN < 0) ? HIGH : LOW);
			delayMicroseconds(2000); // required to allow supply to recover before re-testing
		} while (!i2C_is_released() && downTime < 512000);
		i2c.restart();
		timeOfReset_mS = millis();
		logToSD(" Hard Reset... for ", addr, " took uS:", downTime);
		delayMicroseconds(50000); // delay to light LED
		digitalWrite(RESET_LED_PIN_N, HIGH);
	}
	return resetPerformed;
}

uint8_t initialiseRemoteDisplaysFailed() {
	uint8_t rem_error;
	enum {NO_OF_ATTEMPTS = 2};
	int tryAttemps = NO_OF_ATTEMPTS;
	do {
		rem_error = 0;
		for (int i = 0; i < sizeof(REM_DISPL_ADDR); ++i) {
			if (remoteDisplayIni(REM_DISPL_ADDR[i])) {
				logToSD(" Speedtest Remote Display[", i, "] was:", i2C->getThisI2CFrequency(REM_DISPL_ADDR[i]));
				rem_error = speedTestDevice(REM_DISPL_ADDR[i]);
				logToSD(" Speedtest Remote Display[", i, "] now:", i2C->getThisI2CFrequency(REM_DISPL_ADDR[i]));
				logToSD();
				rem_error = rem_error || remoteDisplayIni(i);
			}
		}
	} while (rem_error && --tryAttemps > 0);
	if (tryAttemps == 0) logToSD("Relay_Run::initialiseRemoteDisplaysFailed() Failed after", NO_OF_ATTEMPTS);
	return rem_error;
}

U1_byte getMixArduinoFailed(I2C_Helper & i2c, int addr) {
	unsigned long waitTime = 3000UL + timeOfReset_mS;

	static bool recursiveCall = false;
	uint8_t hasFailed;
	uint8_t dataBuffa[1] = {1};
	int NO_OF_ATTEMPTS = 4;
	if (recursiveCall || i2c.result.foundDeviceAddr == addr) NO_OF_ATTEMPTS = 1;
	do {
		int tryagain = NO_OF_ATTEMPTS; 
		do {
			hasFailed = i2c.write(addr, 7, 55); // write request temp
			hasFailed |= i2c.read(addr, 7, 1, dataBuffa); // read request temp
			hasFailed |= (dataBuffa[0] != 55);
			if (hasFailed && tryagain == (NO_OF_ATTEMPTS / 2)) {
				if (!recursiveCall) {
					recursiveCall = true;
					hasFailed = speedTestDevice(addr);
				}
				recursiveCall = false;
			}
		} while (hasFailed && --tryagain > 0);

		if (millis() > waitTime && NO_OF_ATTEMPTS - tryagain > 0) {  // don't report if OK first time
			if (tryagain > 0) {
				logToSD("MixArduino OK after" , NO_OF_ATTEMPTS - tryagain + 1);
			} else {
				logToSD("MixArduino Failed after" , NO_OF_ATTEMPTS - tryagain);
			}
		}
	} while (hasFailed && millis() < waitTime);
	return hasFailed;
}

Relay_Run::Relay_Run() {
	pinMode(abs(ZERO_CROSS_PIN), INPUT);
	digitalWrite(abs(ZERO_CROSS_PIN), HIGH); // turn on pullup resistors
	pinMode(abs(RESET_OUT_PIN), OUTPUT);
	digitalWrite(abs(RESET_OUT_PIN), (RESET_OUT_PIN < 0) ? HIGH : LOW);
}

U4_byte Relay_Run::getI2CFrequency() {return I2C_freq;}

bool Relay_Run::getRelayState(bool readFromPort) {
	U1_byte gotData = 0;
	U1_byte deviceAddr = getVal(rDeviceAddr);
	
	if (readFromPort) {
		i2C->read(deviceAddr, REG_8PORT_OPORT, 1, &gotData);
	} else {
		//if (deviceAddr == IO8_PORT_OptCoupl) {
			gotData = optRelayRegister; 
		//} else {
		//	gotData = mixRelayRegister;
		//}
	}
	U1_byte myBit = gotData & (1 << getVal(rPortNo));
	return !(myBit^getVal(rActiveState)); // Relay state 
}

//void Relay_Run::showMem(){
//	//cout << "RelayData: \n";
//	//debugEEprom(epD().getDataStart(),12 * epD().getDataSize());
//	//cout << "Done RelayData. \n";
//}

U1_byte initialisePortsFailed(I2C_Helper & i2c, int addr) { // returns 0 on success or ERR_PORTS_FAILED (1)
	static bool recursiveCall = false;
	U1_byte hasFailed;
	uint8_t dataBuffa[1] = {1};
	int NO_OF_ATTEMPTS = 4;
	if (recursiveCall || i2c.result.foundDeviceAddr == addr) NO_OF_ATTEMPTS = 1;

	int tryagain = NO_OF_ATTEMPTS;
	do {
		hasFailed = i2c.write_at_zero_cross(addr, REG_8PORT_OLAT, Relay_Run::optRelayRegister ); // set latches
		hasFailed |= i2c.write( addr,REG_8PORT_PullUp,0x00); // clear all pull-up resistors
		hasFailed |= i2c.write_at_zero_cross(addr,REG_8PORT_IODIR,0x00); // set all as outputs

		hasFailed |= i2c.read( addr,REG_8PORT_PullUp,1,dataBuffa); 
		hasFailed |= (dataBuffa[0] != 0x00);
		hasFailed |= i2c.read(addr,REG_8PORT_IODIR,1,dataBuffa);
		hasFailed |= (dataBuffa[0] != 0x00);
		if (hasFailed && tryagain == (NO_OF_ATTEMPTS / 2)) {
			if (!recursiveCall) {
				recursiveCall = true;
				hasFailed = speedTestDevice(addr);
			}
			recursiveCall = false;
		}
	} while (hasFailed && --tryagain > 0);

	if (NO_OF_ATTEMPTS - tryagain > 0) { // don't report if OK first time
		if (tryagain > 0) {
			logToSD("Ports OK after" , NO_OF_ATTEMPTS - tryagain + 1);
		} else {
			logToSD("Ports Failed after" , NO_OF_ATTEMPTS - tryagain);
		}
	}
	return hasFailed;
}

signed char Relay_Run::i2cZWrite(uint8_t deviceAddr,uint8_t reg, uint8_t data) {
	//if(switch_mode >= e_znoReads) {
		return i2C->write_at_zero_cross(deviceAddr, reg, data);
	//} else {
		//return i2C->write(deviceAddr, reg, data);
	//}
}

S1_err Relay_Run::writeToRelay(U1_byte deviceAddr, U1_byte data){ // returns 0 success or error codes, e.g.  ERR_PORTS_FAILED & setRelay codes
    S1_err returnVal = 0, iniFailed = 0;
if (i2C->notExists(IO8_PORT_OptCoupl)) return 1;
	bool trySpeedReduction = false;
	noOfErrors = 0;
	//do {
		returnVal = setAndTestRegister(deviceAddr, data);
		if (returnVal != 0) {
		    logToSD("Relay_Run::writeToRelay\tSetAndTestRegister Failed\tTry Ini-Ports",deviceAddr);
			iniFailed = initialisePortsFailed(*i2C,IO8_PORT_OptCoupl);
			trySpeedReduction = true;	
		}
	//} while (returnVal !=0 && iniFailed == 0 && canReduceI2CSpeed(deviceAddr));
	
	if (returnVal !=0 ) {
		if (iniFailed != 0) {
			f->eventS().newEvent(ERR_PORTS_FAILED,deviceAddr);
		    logToSD("Relay_Run::writeToRelay\tInitialisePorts Failed\tERR_PORTS_FAILED",deviceAddr);
		} else {
			f->eventS().newEvent(ERR_I2C_SPEED_RED_FAILED,deviceAddr);
		    logToSD("Relay_Run::writeToRelay\tSpeed Reduction Failed\tERR_I2C_SPEED_RED_FAILED",deviceAddr);
		}
	} else if (trySpeedReduction) {
		f->eventS().newEvent(EVT_I2C_SPEED_RED,deviceAddr);
		logToSD("Relay_Run::writeToRelay\tTry Speed Reduction\tEVT_I2C_SPEED_RED",deviceAddr);
		++reduceSpeedSuccess;	//reportErrors();
	}
	return returnVal; // return 0 for success
}

//bool Relay_Run::canReduceI2CSpeed(int addr) {
//	logToSD("Relay_Run::writeToRelay failed:");
//	if (i2c_slowdown_and_reset(*i2C,addr)) {
//		logToSD("Just tried Speed Reduction");
//		I2C_freq = i2C->getI2CFrequency();
//		return true;
//	} else return false;
//}

S1_err Relay_Run::setAndTestRegister(U1_byte deviceAddr, U1_byte data){ // returns 0 success or error codes, e.g.  ERR_PORTS_FAILED & setRelay codes
    S1_err returnVal =  0; // ERR_READ_VALUE_WRONG; // i2C returns 0=OK, 1=Timeout, 2 = Error during send, 3= NACK during transmit, 4=NACK during complete.
	U1_count i = 0;
	U1_byte gotData, ANDmask = 0xFF, ORmask = 0, outReg = REG_8PORT_OPORT;
	//if (deviceAddr == IO8_PORT_MixValves) {
		//ANDmask = MIX_VALVE_USED;
		//ORmask = data;
		//if (!f->relayR(MIX_VALVE_ENABLE_RELAY).getRelayState()) outReg = REG_8PORT_OLAT;
	//}
	do { // try a few times
		// read port to see if it needs changing
		gotData = 0;
		fixOPDir(deviceAddr);
		switch (switch_mode) {
		case e_alwaysCheck_read_latBeforeWrite:
			returnVal = i2C->read(deviceAddr, REG_8PORT_OLAT, 1, &gotData);
			break;
		case e_alwaysCheck_read_OutBeforeWrite:
			returnVal = i2C->read(deviceAddr, outReg, 1, &gotData);
			break;
		default: returnVal = 0; gotData = ~data;
		}
		//gotData = gotData | ORmask;
		if (returnVal > 0) {
			returnVal = ERR_I2C_READ_FAILED;
			f->eventS().newEvent(ERR_I2C_READ_FAILED,deviceAddr);
			logToSD("Relay_Run::setAndTestRegister\tRead port to see if it needs changing\tERR_I2C_READ_FAILED",deviceAddr);
		} else if((gotData & ANDmask) != (data & ANDmask)) { //Need to write to port
			i2cZWrite(deviceAddr,REG_8PORT_OPORT,data);
			delay(i);
			fixOPDir(deviceAddr);

			switch (switch_mode) {
			case e_onChange_read_LATafter:
			case e_alwaysWrite_read_LATafter:
			case e_alwaysCheck_read_latBeforeWrite:
				returnVal = i2C->read(deviceAddr, REG_8PORT_OLAT, 1, &gotData);
				break;
			case e_onChange_read_OUTafter:
			case e_alwaysCheck_read_OUTafter:
			case e_alwaysCheck_read_OutBeforeWrite:
				returnVal = i2C->read(deviceAddr, outReg, 1, &gotData);
				break;
			default: returnVal = 0; gotData = data; // assume it is OK
			}

			//gotData = gotData | ORmask;
			if(returnVal==0 && (gotData & ANDmask) != (data & ANDmask)) { // write failed, so set registers and try again
				returnVal = ERR_READ_VALUE_WRONG;
				logToSD("Relay_Run::setAndTestRegister\tRead what has been written\tERR_READ_VALUE_WRONG");
				// report current state
				//if (deviceAddr == IO8_PORT_MixValves) {
					//if (!f->relayR(MIX_VALVE_ENABLE_RELAY).getRelayState(true)) {
						//f->eventS().newEvent(ERR_MIX_NOT_ON,deviceAddr);
						//logToSD("Relay_Run::setAndTestRegister\tMix Off - should be on.\tERR_MIX_NOT_ON",deviceAddr);
						//optRelayRegister &= ~(1<<MIX_VALVE_ENABLE_RELAY);
					//}
				//}
				Serial.print("Dev Addr:"); Serial.print(deviceAddr,HEX);
				Serial.print(" Mask:"); Serial.print(ANDmask,HEX);
				i2C->read(deviceAddr, REG_8PORT_IODIR, 1, &gotData);
				Serial.print(" DIR:"); Serial.print(gotData,HEX);
				i2C->read(IO8_PORT_OptCoupl,REG_8PORT_PullUp, 1, &gotData);
				Serial.print(" PULL:"); Serial.print(gotData,HEX);
				Serial.print(" Sent:"); Serial.print(data & ANDmask,HEX);
				i2C->read(deviceAddr, outReg, 1, &gotData);
				Serial.print(" OP:"); Serial.print(gotData & ANDmask,HEX);
				i2C->read(deviceAddr, REG_8PORT_OLAT, 1, &gotData);
				//gotData = gotData | ORmask;
				Serial.print(" LAT:"); Serial.println(gotData & ANDmask,HEX);
				if ((gotData & ANDmask) == (data & ANDmask)) {
					f->eventS().newEvent(ERR_OP_NOT_LAT,deviceAddr);
					logToSD("Relay_Run::setAndTestRegister\tLatch disagrees with OP.\tERR_OP_NOT_LAT",deviceAddr);
					++latch_port_disagree;
				}
				// Reset all registers
				logToSD("Relay_Run::setAndTestRegister\tReset All Registers.\tINI_Ports",deviceAddr);
				initialisePortsFailed(*i2C,IO8_PORT_OptCoupl);
			} // else Write OK - or read failed.
			if (i != 0) ++rewriteSuccess;
		} // else was already OK
		if (returnVal && (i + 1) < NO_OF_RETRIES) {logToSD("Relay_Run::setAndTestRegister Failed. Try again",i);}
	} while (returnVal && ++i < NO_OF_RETRIES);
	if (i > 0 && returnVal==0) {Serial.print(" OK after:");Serial.println(i,DEC);}
	return returnVal;
}

bool Relay_Run::fixOPDir(uint8_t deviceAddr) {
	//if (setRelayMode == e_noDIR) return true;
	i2cZWrite(deviceAddr, REG_8PORT_IODIR, (uint8_t)0); // set all as outputs
	return true;
}

void Relay_Run::reportErrors(){
	//lcd.clear();
	//lcd.noCursor();
	//lcd.setCursor(0,0);
	//lcd.print("Relay Lat/Op:"); 
	//lcd.print(latch_port_disagree); 
	if (latch_port_disagree > 0) {Serial.print("latch_port_disagree:"); Serial.println(latch_port_disagree);latch_port_disagree=0;}
 //	lcd.setCursor(0,1);
	//lcd.print("Write:"); 
	//lcd.print(rewriteSuccess);
	if (rewriteSuccess > 0) {Serial.print("Re-Write:"); Serial.println(rewriteSuccess);rewriteSuccess = 0;}
 //	lcd.setCursor(10,1);
	//lcd.print("vSpd:"); 
	//lcd.print(reduceSpeedSuccess);
	if (reduceSpeedSuccess > 0) {Serial.print("ReduceSpeed:"); Serial.println(reduceSpeedSuccess);reduceSpeedSuccess = 0;}
}

//void Relay_Run::testI2Cdevices() {
//	mainLCD->setCursor(0, 2);
//	mainLCD->print("Test: 0x");
//	int countOK = 0;
//	for (U1_ind i=0; i < f->tempSensors.count(); ++i) {
//		logToSD("Relay_Run::resetSpeed\tTry TS",i);
//		mainLCD->print(f->tempSensorR(i).getVal(0),HEX);
//		f->tempSensorR(i).getSensTemp();
//		mainLCD->print("    ");
//		mainLCD->setCursor(17,2);
//		if (temp_sense_hasError) {
//			mainLCD->print("Bad");
//			delay(2000);
//			resetI2C();
//		} else {
//			++countOK;
//			mainLCD->print(" OK");
//			delay(500);
//		}
//	}
//	//hasFailed = i2C_h->read(IO8_PORT_OptCoupl, 0, 1, dataBuff);
//	//   hasFailed = i2C_h->read(MIX_VALVE_I2C_ADDR, 7, 1, dataBuff); // read request temp
//	//hasFailed = hallLCD->begin(16,2);
//	//hasFailed = usLCD->begin(16,2);
//	//hasFailed = flatLCD->begin(16,2);
//
//	mainLCD->setCursor(0, 3);
//	mainLCD->print("Total:");
//	mainLCD->print(countOK);
//}


S1_err Relay_Run::showSpeedTestFailed(I2C_Helper::TestFnPtr testFn, const char * device) {
	mainLCD->setCursor(5,2);
	mainLCD->print("       0x");
	mainLCD->setCursor(5,2);
	mainLCD->print(device);
	mainLCD->setCursor(14,2);
	mainLCD->print(i2C->result.foundDeviceAddr,HEX);
	mainLCD->print("    ");
	i2C->speedTestNextS(testFn);
	mainLCD->setCursor(17,2);
	if (!i2C->result.error == 0) {
		mainLCD->print("Bad");
		logToSD("Relay_Run::resetSpeed for ", i2C->result.foundDeviceAddr, " Failed");
		delay(2000);
	} else {
		mainLCD->print(" OK");
		logToSD("Relay_Run::resetSpeed for ", i2C->result.foundDeviceAddr, " OK at ", i2C->result.thisHighestFreq);
	}
	logToSD();
	return i2C->result.error;
}

S1_err Relay_Run::resetSpeed(U1_byte deviceAddr,U4_byte maxSpeed){ // returns 0 for success or returns ERR_PORTS_FAILED / ERR_I2C_SPEED_FAIL
	enum {e_allPassed, e_mixValve, e_relays=2, e_US_Remote=4, e_FL_Remote=8, e_DS_Remote=16, e_TempSensors=32};
	i2C->setTimeoutFn(hardReset_Performed);
	S1_err returnVal = 0;
	U4_byte newFreq, iniFreq = I2C_freq;
	mainLCD->setCursor(0,2);
	mainLCD->print("Test ");
	logToSD();
	logToSD("Relay_Run::resetSpeed has been called");
	i2C->result.reset();
	
	logToSD("Relay_Run::resetSpeed\tTry Relays");
	i2C->result.foundDeviceAddr = IO8_PORT_OptCoupl;
	if (showSpeedTestFailed(initialisePortsFailed, "Relay")) {
		f->eventS().newEvent(ERR_PORTS_FAILED,0);
        returnVal = ERR_PORTS_FAILED;
	}

	logToSD("Relay_Run::resetSpeed\tTry Remotes");
	i2C->result.foundDeviceAddr = DS_REMOTE_ADDRESS;
	if(showSpeedTestFailed(remoteDisplayFailed, "DS Rem")) {
		f->eventS().newEvent(ERR_I2C_READ_FAILED,0);
        returnVal = ERR_I2C_READ_FAILED;
	}
	
	i2C->result.foundDeviceAddr = US_REMOTE_ADDRESS;
	if(showSpeedTestFailed(remoteDisplayFailed, "US Rem")) {
		f->eventS().newEvent(ERR_I2C_READ_FAILED,0);
        returnVal = ERR_I2C_READ_FAILED;
	}
	
	i2C->result.foundDeviceAddr = FL_REMOTE_ADDRESS;
	if(showSpeedTestFailed(remoteDisplayFailed, "FL Rem")) {
		f->eventS().newEvent(ERR_I2C_READ_FAILED,0);
        returnVal = ERR_I2C_READ_FAILED;
	}

	logToSD("Relay_Run::resetSpeed\tTry Mix Valve");
	i2C->result.foundDeviceAddr = MIX_VALVE_I2C_ADDR;
	if(showSpeedTestFailed(getMixArduinoFailed, "Mix V")) {
		f->eventS().newEvent(ERR_MIX_ARD_FAILED,0);
        returnVal = ERR_MIX_ARD_FAILED;
	}

	for (U1_ind i=0; i < f->tempSensors.count(); ++i) {
		logToSD("Relay_Run::resetSpeed\tTry TS",i);
		i2C->result.foundDeviceAddr = f->tempSensors[i].getVal(0);
		showSpeedTestFailed(tempSensorFailed, "TS");
	}

	newFreq = i2C->result.maxSafeSpeed;
	mainLCD->setCursor(12,2);
	mainLCD->print(newFreq);
	logToSD("I2C Max Speed is ", (int)newFreq);
	//if (newFreq > i2C->MIN_I2C_FREQ) {
		//if (maxSpeed > 0 && newFreq >= maxSpeed) newFreq = maxSpeed;
		//I2C_freq = newFreq;
		//i2C->setI2CFrequency(newFreq);
		//returnVal = 0;
	//} else returnVal = ERR_I2C_SPEED_FAIL;
	//logToSD("I2C Speed is ", (int)newFreq);
	i2C->setTimeoutFn(resetI2C);
	i2C->result.reset();
	return returnVal;
}

S1_err Relay_Run::refreshRelays() {
	S1_err returnVal = 0;
	//if (temp_sense_hasError) {
		//logToSD("Relay_Run::refreshRelays\ttemp_sense_hasError");
		//resetI2C(*i2C,0);
	//}
	
	bool alwaysWrite = (switch_mode != e_onChange_read_LATafter && switch_mode != e_onChange_read_OUTafter);
	if (alwaysWrite || prevOptRelayRegister != optRelayRegister) {
		prevOptRelayRegister = optRelayRegister;
		returnVal = writeToRelay(IO8_PORT_OptCoupl,optRelayRegister);
	}
	//if (alwaysWrite || prevMixRelayRegister != mixRelayRegister) {
		//prevMixRelayRegister = mixRelayRegister;
		//returnVal =  returnVal | writeToRelay(IO8_PORT_MixValves,mixRelayRegister);
	//}
	if (returnVal) { //failed and tried resetting speed.
		f->eventS().newEvent(ERR_PORTS_FAILED,0);
		logToSD("Relay_Run::refreshRelays\tNo Relays\tERR_PORTS_FAILED");
	}
	
	return returnVal;
}

bool Relay_Run::setRelay(U1_byte state) { // returns true if state is changed
	U1_byte * oState;
	U1_byte deviceAddr = getVal(rDeviceAddr);

	//if (deviceAddr == IO8_PORT_OptCoupl) {
		oState = &optRelayRegister; 
	//} else {
	//	oState = &mixRelayRegister;
	//}
	U1_byte myBitMask = 1 << getVal(rPortNo);
	U1_byte myBit = (*oState  & myBitMask) != 0;
	U1_byte currState = !(myBit^getVal(rActiveState));

    myBit = !(state^getVal(rActiveState)); // Required bit state 
    if (!myBit) { // clear bit
        *oState = *oState & ~myBitMask;
    } else { // set this bit
        *oState = *oState | myBitMask;
    }
    return currState != state; // returns true if state is changed
}

S1_err Relay_Run::testRelays() {
    /////////////////////// Cycle through ports /////////////////////////
	S1_err returnVal = 0, noOfRelays = f->relays.count();
	U1_count numberFailed = 0;
	logToSD("Relay_Run::testRelays");
	if (initialisePortsFailed(*i2C,IO8_PORT_OptCoupl)) {
		f->eventS().newEvent(ERR_PORTS_FAILED,0);
		logToSD("Relay_Run::testRelays\tNo Relays\tERR_PORTS_FAILED");
		return noOfRelays;
	}
	mainLCD->setCursor(0,3);
	mainLCD->print("Relay Test: ");
	for (U1_count relayNo = 0; relayNo < noOfRelays; ++relayNo){ // 12 relays, but 1 is mix enable.
		f->relayR(RELAY_ORDER[relayNo]).setRelay(1);
		returnVal = refreshRelays();
		delay(200);
		f->relayR(RELAY_ORDER[relayNo]).setRelay(0);
		returnVal = refreshRelays();
		numberFailed = numberFailed + (returnVal != 0);
		mainLCD->setCursor(12,3);
		mainLCD->print(relayNo,DEC);
		if (returnVal) {
			f->eventS().newEvent(ERR_PORTS_FAILED,relayNo);
		    logToSD("Relay_Run::testRelays\tRelay Failed\tERR_PORTS_FAILED",relayNo);
			mainLCD->print(" Fail");
		} else {
			mainLCD->print(" OK  ");
		}
		delay(200);
		returnVal = 0;
	}
	mainLCD->setCursor(12,3);
	if (numberFailed > 0) {
		mainLCD->print(numberFailed); mainLCD->print("Failed");
	} else {
		mainLCD->print("All OK  ");
		I2C_OK = true;
	}
	delay(500);
	return numberFailed;
}

uint8_t remoteDisplayFailed(I2C_Helper & i2c, int addr) { // returns 0 for success
	uint8_t dataBuffa[2];
	uint16_t & thisKey = reinterpret_cast<uint16_t &>(dataBuffa[0]); 
	thisKey  = 0x5C36;
	uint8_t hasFailed = i2c.write(addr, 0x04, 2, dataBuffa); // Write to GPINTEN
	thisKey = 0;
	hasFailed = i2c.read(addr, 0x04, 2, dataBuffa); // Check GPINTEN is correct
	return hasFailed | (thisKey != 0x5C36);
}

uint8_t remoteDisplayIni(int addr) {
	uint8_t hasFailed = 0;
	switch (addr) {
	case DS_REMOTE_ADDRESS:
		hall_display->resetDisplay();
		if (hallLCD) hasFailed = hallLCD->begin(16, 2);
		if (hasFailed) {
			logToSD("Hall LCD failed at",hasFailed);
		} else logToSD("Hall LCD begin() OK");
		break;
	case FL_REMOTE_ADDRESS: 
		flat_display->resetDisplay();
		if (flatLCD) hasFailed = flatLCD->begin(16, 2);
		if (hasFailed) {
			logToSD("Flat LCD failed at",hasFailed);
		} else logToSD("Flat LCD begin() OK");
		break;
	case US_REMOTE_ADDRESS:
		us_display->resetDisplay();
		if (usLCD) hasFailed = usLCD->begin(16, 2);
		if (hasFailed) {
			logToSD("US LCD failed at",hasFailed);
		} else logToSD("US LCD begin() OK");
		break;
	}
	return hasFailed;
}

uint8_t tempSensorFailed(I2C_Helper & i2c, int addr) {
	uint8_t hasFailed;
	uint8_t temp[2];
	hasFailed = i2c.read(addr, DS75LX_HYST_REG, 2, temp);
	hasFailed |= (temp[0] != 75);
	hasFailed |= i2c.read(addr, DS75LX_LIMIT_REG, 2, temp);
	hasFailed |= (temp[0] != 80);
	return hasFailed;
}