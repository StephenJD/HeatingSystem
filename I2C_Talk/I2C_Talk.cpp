#include "I2C_Talk.h"
//#include <Logging.h>

#ifndef VARIANT_MCK
#define VARIANT_MCK F_CPU
#endif

//void log(const char * msg, long val);
//void log(const char * msg, long val, const char * name, long val2 = 0xFFFFABCD);

// encapsulation is improved by using global vars and functions in .cpp file
// rather than declaring these as class statics in the header file,
// since any changes to these would "dirty" the header file unnecessarily.

// private global variables //
static const uint16_t HALF_MAINS_PERIOD = 10000; // in microseconds. 10000 for 50Hz, 8333 for 60Hz

//void log(const char * msg);

// Private free function prototypes
bool g_delayTrue(uint8_t millisecs);
uint32_t g_timeSince(uint32_t startTime);

int8_t I2C_Talk::s_zeroCrossPin;
uint16_t I2C_Talk::s_zxSigToXdelay; // microseconds

I2C_Talk * I2C_Talk::I_I2Cdevice::_i2C = 0;
int8_t I2C_Talk::TWI_BUFFER_SIZE = 32;

// Public I2C_Talk functions
I_I2Cdevice I2C_Talk::makeDevice(uint8_t addr) { return I_I2Cdevice(this, addr); }


I2C_Talk::I2C_Talk(TwoWire &wire_port, int32_t i2cFreq)
	: successAfterRetries(0),
	wire_port(wire_port),
	noOfRetries(5),
	timeoutFunctor(0),
	relayDelay(0),
	relayStart(0),
	_myAddress(_single_master),
	_canWrite(true)
	//_I2C_DATA_PIN(wire_port == Wire ? 20 : 70)
	{
	TWI_BUFFER_SIZE = getTWIbufferSize();
	restart();
	setI2CFrequency(i2cFreq);
	s_zeroCrossPin = 0;
	s_zxSigToXdelay = 700;
	_lastRestartTime = micros();
	}
	
I2C_Talk::I2C_Talk(int multiMaster_MyAddress, TwoWire &wire_port, int32_t i2cFreq) 
	: successAfterRetries(0),
	wire_port(wire_port),
	noOfRetries(5),
	timeoutFunctor(0),
	relayDelay(0),
	relayStart(0),
	_myAddress(multiMaster_MyAddress),
	_canWrite(true)
	{
	//Serial.println("Create I2C_Talk - multiMaster");
	TWI_BUFFER_SIZE = getTWIbufferSize();
	restart();
	setI2CFrequency(i2cFreq);
	s_zeroCrossPin = 0;
	s_zxSigToXdelay = 700;
	_lastRestartTime = micros();
    }

I2C_Talk::I2C_Talk(TwoWire &wire_port, int8_t zxPin, uint8_t retries, uint16_t zxDelay, I_I2CresetFunctor * timeOutFn, int32_t i2cFreq)
	: successAfterRetries(0),
	wire_port(wire_port),
	noOfRetries(retries),
	timeoutFunctor(timeOutFn),
	relayDelay(0),
	relayStart(0),
	_myAddress(_single_master),
	_canWrite(true)
	{
	//log("Construct I2C_Talk - singleMaster");
	TWI_BUFFER_SIZE = getTWIbufferSize();
	restart();
	setI2CFrequency(i2cFreq);
	s_zeroCrossPin = zxPin;
	s_zxSigToXdelay = zxDelay;
	_lastRestartTime = micros();
	}

uint8_t I2C_Talk::notExists(I2C_Talk & i2c, int deviceAddr) {
	return i2c.notExists(deviceAddr);
}

uint8_t I2C_Talk::notExists(int deviceAddr) {
	uint8_t result = beginTransmission(deviceAddr);
	if (result == _OK) {
		return check_endTransmissionOK(deviceAddr);
	} else return result;
}

uint8_t I2C_Talk::read(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, uint8_t *dataBuffer) {
	uint8_t i = 0, returnStatus;
	if (!_canWrite) {
		//Serial.println("I2C_Talk::read slave_shouldnt_write");
		return _slave_shouldnt_write;
	}
	do {
		waitForDeviceReady(deviceAddr);
		returnStatus = beginTransmission(deviceAddr);
		if (returnStatus ==  _OK) {
			wire_port.write(registerAddress);
			returnStatus = getData(deviceAddr, numberBytes, dataBuffer);
		} else return returnStatus;
	} while (returnStatus && ++i < noOfRetries && restart("read 0x",deviceAddr) && g_delayTrue(i));// create an increasing delay if we are going to loop
	if (i < noOfRetries && i > successAfterRetries) successAfterRetries = i;
	return returnStatus;
}

uint8_t I2C_Talk::readEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, uint8_t *dataBuffer) {
	uint8_t returnStatus;
	while (numberBytes > 0) {
		uint8_t bytesOnThisPage = min(numberBytes, TWI_BUFFER_SIZE);
		waitForDeviceReady(deviceAddr);
		returnStatus = beginTransmission(deviceAddr);
		if (returnStatus == _OK) {
			wire_port.write(pageAddress >> 8);   // Address MSB
			wire_port.write(pageAddress & 0xff); // Address LSB
			returnStatus = getData(deviceAddr, bytesOnThisPage, dataBuffer);
		}
		else return returnStatus;
		pageAddress += bytesOnThisPage;
		dataBuffer += bytesOnThisPage;
		numberBytes -= bytesOnThisPage;
	}
	return returnStatus;
}

uint8_t I2C_Talk::getData(uint16_t deviceAddr, uint16_t numberBytes, uint8_t *dataBuffer) {
	uint8_t returnStatus = check_endTransmissionOK(deviceAddr);
	//retuns 0=_OK, 1=_Insufficient_data_returned, 2=_NACK_during_address_send, 3=_NACK_during_data_send, 4=_NACK_during_complete, 5=_NACK_receiving_data, 6=_Timeout, 7=_slave_shouldnt_write, 8=_I2C_not_created
	if (returnStatus == _OK) {
		returnStatus = (wire_port.requestFrom((int)deviceAddr, (int)numberBytes) != numberBytes);
		if (returnStatus != _OK) { // returned error
			returnStatus = wire_port.read(); // retrieve error code
			//log("I2C_Talk::read err: for ", long(deviceAddr), getError(returnStatus),long(i));
			if (returnStatus == _Timeout) { callTime_OutFn(deviceAddr); }
		}
		else {
			for (uint8_t i = 0; wire_port.available(); ++i) {
				dataBuffer[i] = wire_port.read();
			}
			//Serial.println("OK");
			//Serial.print("Req: "); Serial.print(numberBytes);
			//Serial.print("Got: "); Serial.println(i);
		}
	}
	else {
		//if (i > 0) {Serial.print(i,DEC);Serial.print(" Error Reading addr:0x");}
		//Serial.println(returnStatus);
	}
	return returnStatus;
}

uint8_t I2C_Talk::write(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, const uint8_t * dataBuffer) {
	uint8_t i = 0, returnStatus;
	if (!_canWrite) return _slave_shouldnt_write;
	do{
		returnStatus = beginTransmission(deviceAddr);
		if (returnStatus == _OK) {
			wire_port.write(registerAddress);
			wire_port.write(dataBuffer, uint8_t(numberBytes));
			returnStatus = check_endTransmissionOK(deviceAddr);
			_lastWrite = micros();
			if (returnStatus) {
				//log(" I2C_Talk::write Error Writing addr: ",long(deviceAddr), getError(returnStatus),long(i)); 
				//Serial.print(i,DEC); 
				//Serial.print(deviceAddr,HEX); 
				//Serial.print(" Reg:0x"); 
				//Serial.print(registerAddress,HEX);
				//Serial.println(getError(returnStatus));
			}
			relayDelay = g_timeSince(relayStart);	
		} else return returnStatus;
	} while (returnStatus && ++i < noOfRetries && restart("write 0x",deviceAddr) && g_delayTrue(i));// create an increasing delay if we are going to loop
	if (i < noOfRetries && i > successAfterRetries) successAfterRetries = i;
	return(returnStatus);
}

uint8_t I2C_Talk::write_verify(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer) {
	uint8_t verifyBuffer[32];
	auto error = write(deviceAddr, registerAddress, numberBytes, dataBuffer);
	if (error == _OK) error = read(deviceAddr, registerAddress, numberBytes, verifyBuffer);
	if (error == _OK) {
		for (int i = 0; i < numberBytes; ++i) {
			error |= (verifyBuffer[i] == dataBuffer[i] ? _OK : _I2C_ReadDataWrong);
		}
	}
	return error;
}

uint8_t I2C_Talk::write(uint16_t deviceAddr, uint8_t registerAddress, uint8_t data) {
	return write(deviceAddr, registerAddress, 1, &data);
}

uint8_t I2C_Talk::write(const uint8_t *dataBuffer, uint16_t numberBytes) {
	//Serial.println("Slave Write");
	return (uint8_t) wire_port.write(dataBuffer, uint8_t(numberBytes));
}

uint8_t I2C_Talk::writeEP(uint16_t deviceAddr, int pageAddress, uint8_t data) {
	return writeEP(deviceAddr, pageAddress, 1, &data);
}

uint8_t I2C_Talk::writeEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, const uint8_t *dataBuffer) {
	uint8_t returnStatus;
	while (numberBytes > 0) {
		uint8_t bytesUntilPageBoundary = I2C_EEPROM_PAGESIZE - pageAddress % I2C_EEPROM_PAGESIZE;
		uint8_t bytesOnThisPage = min(numberBytes, bytesUntilPageBoundary);
		bytesOnThisPage = min(bytesOnThisPage, TWI_BUFFER_SIZE-2);
		waitForDeviceReady(deviceAddr);
		beginTransmission(deviceAddr);
		wire_port.write(pageAddress >> 8);   // Address MSB
		wire_port.write(pageAddress & 0xff); // Address LSB
		wire_port.write(dataBuffer, bytesOnThisPage);
		returnStatus = check_endTransmissionOK(deviceAddr);
		_lastWrite = micros();
		pageAddress += bytesOnThisPage;
		dataBuffer += bytesOnThisPage;
		numberBytes -= bytesOnThisPage;
	}
	return(returnStatus);
}

void I2C_Talk::waitForDeviceReady(uint16_t deviceAddr)
{
#define I2C_WRITEDELAY  5000

	// Wait until EEPROM gives ACK again.
	// this is a bit faster than the hardcoded 5 milliSeconds
	while ((micros() - _lastWrite) <= I2C_WRITEDELAY)
	{
		beginTransmission(deviceAddr);
		if (check_endTransmissionOK(deviceAddr) == _OK) break;
	}
}

int32_t I2C_Talk::setI2Cfreq_retainAutoSpeed(int32_t i2cFreq) {
	if (_i2cFreq != i2cFreq) {
		_i2cFreq = i2cFreq;
		if (i2cFreq < MIN_I2C_FREQ) _i2cFreq = MIN_I2C_FREQ;
		if (i2cFreq > MAX_I2C_FREQ) _i2cFreq = MAX_I2C_FREQ;
		wire_port.setClock(_i2cFreq);
	}
	return _i2cFreq;
}

int32_t I2C_Talk::setI2CFrequency(int32_t i2cFreq) { // turns auto-speed off
	useAutoSpeed(false);
	return setI2Cfreq_retainAutoSpeed(i2cFreq);
}

void I2C_Talk::setNoOfRetries(uint8_t retries) {noOfRetries = retries;}
uint8_t I2C_Talk::getNoOfRetries() {return noOfRetries;}
void I2C_Talk::setTimeoutFn (I_I2CresetFunctor * timeOutFn) {timeoutFunctor = timeOutFn;} // Timeout function pointer

void I2C_Talk::setTimeoutFn (TestFnPtr timeOutFn) { // convert function into functor
	_i2CresetFunctor.set(timeOutFn);
	timeoutFunctor = &_i2CresetFunctor;
} // Timeout function pointer

void I2C_Talk::setZeroCross(int8_t zxPin) {s_zeroCrossPin = zxPin;} // Arduino pin signalling zero-cross detected. 0 = disable, +ve = signal on rising edge, -ve = signal on falling edge
void I2C_Talk::setZeroCrossDelay(uint16_t zxDelay) {s_zxSigToXdelay = zxDelay;} // microseconds delay between signal on zero_cross_pin to next true zero-cross.

const char * I2C_Talk::getError(int errorCode) {
 // error_codes {_OK, _Insufficient_data_returned, _NACK_during_address_send, _NACK_during_data_send, _NACK_during_complete, _NACK_receiving_data, _Timeout, _speedError, _slave_shouldnt_write, _I2C_not_created };
	switch (errorCode) {
	case _OK:	return (" No Error");
	case _Insufficient_data_returned:	return (" Insufficient data returned");
	case _NACK_during_address_send:	return (" NACK address send");
	case _NACK_during_data_send:	return (" NACK data send");
	case _NACK_during_complete:	return (" NACK during complete");
	case _NACK_receiving_data:	return (" NACK receiving data");
	case _Timeout:	return (" Timeout");
	case _speedError:	return (" Speed Test Disabled for this device. Enable with setThisI2CFrequency()");
	case _slave_shouldnt_write:	return (" Slave Shouldn't Write");
	case _I2C_not_created:	return (" I2C not created");
	case _I2C_Device_Not_Found: return (" I2C Device not found");
	case _I2C_ReadDataWrong: return (" I2C Read Data Wrong");
	case _I2C_AddressOutOfRange: return (" I2C Address Out of Range");
	default: return (" Not known");
	}
}

uint8_t I2C_Talk::receiveFromMaster(int howMany, uint8_t *dataBuffer) {
	uint8_t noReceived = 0;
	while (wire_port.available() && noReceived < howMany ) {
		dataBuffer[noReceived] = wire_port.read();
		++noReceived;
	}
	return noReceived;
}

// Private Functions
uint8_t I2C_Talk::beginTransmission(uint16_t deviceAddr) { // return false to inhibit access
	if (deviceAddr > 127) return _I2C_AddressOutOfRange;
	if (getThisI2CFrequency(deviceAddr) == 0) {
		if (millis() - getFailedTime(deviceAddr) > 600000) {
			setThisI2CFrequency(deviceAddr, 400000);
		}
		else return _speedError;
	}	
	wire_port.beginTransmission((uint8_t)deviceAddr);
	return _OK;
}

uint8_t I2C_Talk::check_endTransmissionOK(int addr) { // returns 0=OK, 1=Timeout, 2 = Error during send, 3= NACK during transmit, 4=NACK during complete.
	//Serial.println("endTransmission()");
	if (usingAutoSpeed()) {
		setI2Cfreq_retainAutoSpeed(getThisI2CFrequency(addr)); // set device-specific frequency if available
		//Serial.print("Speed for addr: 0x"); Serial.print(deviceAddr, HEX); Serial.print(" Freq: "); Serial.println(getI2CFrequency());
	}
	if (_waitForZeroCross) waitForZeroCross();
	uint8_t error = wire_port.endTransmission();
	if (error && usingAutoSpeed()) /*slowdown_and_reset(addr)*/;
	//if (error) {
	//	logger().log("check_endTransmissionOK() error:", error, "Auto:",  usingAutoSpeed());
	//	logger().log("Time since error:", millis() - getFailedTime(addr), "Addr:", addr);
	//}
	if (error == _Timeout) {
		//log("check_endTransmissionOK()_Timeout: calling timeoutFn for: dec",addr);
		callTime_OutFn(addr);
	} 
	//else if (error != _OK) {
		//Serial.print("check_endTransmissionOK() failure: "); Serial.println(getError(error));
	//}
	return error;
}

void I2C_Talk::waitForZeroCross(){
#if !defined (ZPSIM)
	if (s_zeroCrossPin) { //////////// Zero Cross Detection function ////////////////////////
		uint32_t fireTime = micros();
		while (digitalRead(abs(s_zeroCrossPin)) == (s_zeroCrossPin>0) && g_timeSince(fireTime) < HALF_MAINS_PERIOD); // We need non-active state.
		fireTime = micros();
		while (digitalRead(abs(s_zeroCrossPin)) == (s_zeroCrossPin<0) && g_timeSince(fireTime) < HALF_MAINS_PERIOD); // Now wait for active state.
		fireTime = micros();
		if (s_zxSigToXdelay < relayDelay) fireTime = fireTime + HALF_MAINS_PERIOD;
		fireTime = fireTime + s_zxSigToXdelay - relayDelay;
		while (micros() < fireTime); // wait for fireTime.
		relayStart = micros(); // time relay delay
	}
	_waitForZeroCross = false;
#endif
}

bool I2C_Talk::restart() {
	wire_port.begin(_myAddress);
	return true;
}

bool I2C_Talk::restart(const char * name,int addr) {
	restart();
	//Serial.print("I2C::restart() "); Serial.print(name);Serial.println(addr, HEX);
	return true;
}

uint8_t I2C_Talk::getTWIbufferSize() {
	uint8_t junk[1];
	return uint8_t(wire_port.write(junk, 100));
}

// *****************************************************************************************
// ***************************   I2C_Helper_Auto_Speed_Hoist  ******************************
// *****************************************************************************************

int I2C_Helper_Auto_Speed_Hoist::_findDevice(int16_t devAddr, int8_t * devAddrArr, int noOfDevices) {
	int index = 0;
	do {
		if (devAddrArr[index] == 0) { devAddrArr[index] = static_cast<signed char>(devAddr); break; }
		if (devAddrArr[index] == devAddr) break;
	} while (++index < noOfDevices) ;

	return index;
}

int32_t I2C_Helper_Auto_Speed_Hoist::_getI2CFrequency(int16_t devAddr, int8_t * devAddrArr, const int32_t * i2c_speedArr, int noOfDevices) {
	int index = _findDevice(devAddr, devAddrArr, noOfDevices);
	if (index == noOfDevices) return getI2CFrequency();
	return i2c_speedArr[index];
}

unsigned long I2C_Helper_Auto_Speed_Hoist::_getFailedTime(int16_t devAddr, int8_t * devAddrArr, const unsigned long * failedTimeArr, int noOfDevices) {
	int index = _findDevice(devAddr, devAddrArr, noOfDevices);
	if (index == noOfDevices) return 0;
	return failedTimeArr[index];
}

int32_t I2C_Helper_Auto_Speed_Hoist::_setI2CFrequency(int16_t devAddr, int32_t i2cFreq, int8_t * devAddrArr, int32_t * i2c_speedArr, int noOfDevices) {
	int index = _findDevice(devAddr, devAddrArr, noOfDevices);
	if (index == noOfDevices) return getI2CFrequency();
	i2c_speedArr[index] = i2cFreq;
	return setI2Cfreq_retainAutoSpeed(i2cFreq);
}

void I2C_Helper_Auto_Speed_Hoist::_setFailedTime(int16_t devAddr, int8_t * devAddrArr, unsigned long * failedTimeArr, int noOfDevices) {
	int index = _findDevice(devAddr, devAddrArr, noOfDevices);
	if (index == noOfDevices) return;
	failedTimeArr[index] = millis();
}

// ************** Private Free Functions ************
bool g_delayTrue(uint8_t millisecs){ // wrapper to get a return true for use in short-circuited test
	delay(millisecs);
	return true;
}

uint32_t g_timeSince(uint32_t startTime){
	uint32_t endTime = micros();
	if (endTime < startTime) // Roll-over to zero has occurred - approx every 70 mins.
	return (uint32_t)-1 - startTime + endTime; // +1 to be strictly accurate - but who cares?
	else return endTime - startTime;
}

