#include "I2C_Talk.h"
//#include <Logging.h>

#ifndef VARIANT_MCK
#define VARIANT_MCK F_CPU
#endif

using namespace I2C_Talk_ErrorCodes;

//void log(const char * msg, long val);
//void log(const char * msg, long val, const char * name, long val2 = 0xFFFFABCD);

// encapsulation is improved by using global vars and functions in .cpp file
// rather than declaring these as class statics in the header file,
// since any changes to these would "dirty" the header file unnecessarily.

// private global variables //
static const uint16_t HALF_MAINS_PERIOD = 10000; // in microseconds. 10000 for 50Hz, 8333 for 60Hz

//void log(const char * msg);
uint32_t g_timeSince(uint32_t startTime);

int8_t I2C_Talk::s_zeroCrossPin = 0;
uint16_t I2C_Talk::s_zxSigToXdelay = 700; // microseconds

int8_t I2C_Talk::TWI_BUFFER_SIZE = 32;

I2C_Talk::I2C_Talk(TwoWire & wire_port, int32_t i2cFreq)
	:
	_wire_port(wire_port)
	,_i2cFreq(i2cFreq)
	{	// Locks up if Serial.print or logging() called before setup()
		//wire_port.begin(); //locks up if called before entering setup()
		TWI_BUFFER_SIZE = getTWIbufferSize();
	}

I2C_Talk::I2C_Talk(int multiMaster_MyAddress, TwoWire & wire_port, int32_t i2cFreq) 
	: 
	_wire_port(wire_port)
	,_i2cFreq(i2cFreq)
	,_myAddress(multiMaster_MyAddress)
	{
		//restart(); //locks up if called before entering setup()
		TWI_BUFFER_SIZE = getTWIbufferSize();
    }

I2C_Talk::I2C_Talk(TwoWire &wire_port, int8_t zxPin, uint16_t zxDelay, I2C_Recover * recovery, int32_t i2cFreq)
	: 
	_wire_port(wire_port)
	,_i2cFreq(i2cFreq)
	{
	//restart(); //locks up if called before entering setup()
	s_zeroCrossPin = zxPin;
	s_zxSigToXdelay = zxDelay;
	TWI_BUFFER_SIZE = getTWIbufferSize();
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
	if (!_canWrite) {
		//Serial.println("I2C_Talk::read slave_shouldnt_write");
		return _slave_shouldnt_write;
	}

	waitForDeviceReady(deviceAddr);
	uint8_t returnStatus = beginTransmission(deviceAddr);
	if (returnStatus ==  _OK) {
		_wire_port.write(registerAddress);
		returnStatus = getData(deviceAddr, numberBytes, dataBuffer);
	}
	return returnStatus;
}

uint8_t I2C_Talk::readEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, uint8_t *dataBuffer) {
	uint8_t returnStatus = _OK;
	while (numberBytes > 0) {
		uint8_t bytesOnThisPage = min(numberBytes, TWI_BUFFER_SIZE);
		waitForDeviceReady(deviceAddr);
		returnStatus = beginTransmission(deviceAddr);
		if (returnStatus == _OK) {
			_wire_port.write(pageAddress >> 8);   // Address MSB
			_wire_port.write(pageAddress & 0xff); // Address LSB
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
		returnStatus = (_wire_port.requestFrom((int)deviceAddr, (int)numberBytes) != numberBytes);
		if (returnStatus != _OK) { // returned error
			returnStatus = _wire_port.read(); // retrieve error code
			//log("I2C_Talk::read err: for ", long(deviceAddr), getStatusMsg(returnStatus),long(i));
		}
		else {
			for (uint8_t i = 0; _wire_port.available(); ++i) {
				dataBuffer[i] = _wire_port.read();
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
	uint8_t returnStatus;
	if (!_canWrite) return _slave_shouldnt_write;

	returnStatus = beginTransmission(deviceAddr);
	if (returnStatus == _OK) {
		_wire_port.write(registerAddress);
		_wire_port.write(dataBuffer, uint8_t(numberBytes));
		returnStatus = check_endTransmissionOK(deviceAddr);
		_lastWrite = micros();
		if (returnStatus) {
			//log(" I2C_Talk::write Error Writing addr: ",long(deviceAddr), getStatusMsg(returnStatus),long(i)); 
			//Serial.print(i,DEC); 
			//Serial.print(deviceAddr,HEX); 
			//Serial.print(" Reg:0x"); 
			//Serial.print(registerAddress,HEX);
			//Serial.println(getStatusMsg(returnStatus));
		}
		relayDelay = g_timeSince(relayStart);	
	}
	return returnStatus;
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

uint8_t I2C_Talk::writeEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, const uint8_t *dataBuffer) {
	uint8_t returnStatus = _OK;
	while (numberBytes > 0) {
		uint8_t bytesUntilPageBoundary = I2C_EEPROM_PAGESIZE - pageAddress % I2C_EEPROM_PAGESIZE;
		uint8_t bytesOnThisPage = min(numberBytes, bytesUntilPageBoundary);
		bytesOnThisPage = min(bytesOnThisPage, TWI_BUFFER_SIZE-2);
		waitForDeviceReady(deviceAddr);
		beginTransmission(deviceAddr);
		_wire_port.write(pageAddress >> 8);   // Address MSB
		_wire_port.write(pageAddress & 0xff); // Address LSB
		_wire_port.write(dataBuffer, bytesOnThisPage);
		returnStatus = check_endTransmissionOK(deviceAddr);
		_lastWrite = micros();
		pageAddress += bytesOnThisPage;
		dataBuffer += bytesOnThisPage;
		numberBytes -= bytesOnThisPage;
	}
	return(returnStatus);
}

uint8_t I2C_Talk::write(const uint8_t *dataBuffer, uint16_t numberBytes) { 
	return (uint8_t)_wire_port.write(dataBuffer, uint8_t(numberBytes)); 
} // Called by slave in response to request from a Master. Return errCode.


void I2C_Talk::waitForDeviceReady(uint16_t deviceAddr)
{
	constexpr decltype(micros()) I2C_WRITEDELAY = 5000;

	// Wait until EEPROM gives ACK again.
	// this is a bit faster than the hardcoded 5 milliSeconds
	while ((micros() - _lastWrite) <= I2C_WRITEDELAY)
	{
		beginTransmission(deviceAddr);
		if (check_endTransmissionOK(deviceAddr) == _OK) break;
	}
}

int32_t I2C_Talk::setI2CFrequency(int32_t i2cFreq) {
	if (_i2cFreq != i2cFreq) {
		_i2cFreq = i2cFreq;
		if (i2cFreq < MIN_I2C_FREQ) _i2cFreq = MIN_I2C_FREQ;
		if (i2cFreq > MAX_I2C_FREQ) _i2cFreq = MAX_I2C_FREQ;
		_wire_port.setClock(_i2cFreq);
	}
	return _i2cFreq;
}

void I2C_Talk::setZeroCross(int8_t zxPin) {s_zeroCrossPin = zxPin;} // Arduino pin signalling zero-cross detected. 0 = disable, +ve = signal on rising edge, -ve = signal on falling edge
void I2C_Talk::setZeroCrossDelay(uint16_t zxDelay) {s_zxSigToXdelay = zxDelay;} // microseconds delay between signal on zero_cross_pin to next true zero-cross.

const char * I2C_Talk::getStatusMsg(int errorCode) {
 // error_codes {_OK, _Insufficient_data_returned, _NACK_during_address_send, _NACK_during_data_send, _NACK_during_complete, _NACK_receiving_data, _Timeout, _disabledDevice, _slave_shouldnt_write, _I2C_not_created };
	switch (errorCode) {
	case _OK:	return (" No Error");
	case _Insufficient_data_returned:	return (" Insufficient data returned");
	case _NACK_during_address_send:	return (" NACK address send");
	case _NACK_during_data_send:	return (" NACK data send");
	case _NACK_during_complete:	return (" NACK during complete");
	case _NACK_receiving_data:	return (" NACK receiving data");
	case _Timeout:	return (" Timeout");
	case _disabledDevice:	return (" Device Disabled. Enable with set_runSpeed()");
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
	while (_wire_port.available() && noReceived < howMany ) {
		dataBuffer[noReceived] = _wire_port.read();
		++noReceived;
	}
	return noReceived;
}

// Private Functions
uint8_t I2C_Talk::beginTransmission(uint16_t deviceAddr) { // return false to inhibit access
	uint8_t error = addressOutOfRange(deviceAddr);
	if (error == _OK) _wire_port.beginTransmission((uint8_t)deviceAddr);
	return error;
}

uint8_t I2C_Talk::check_endTransmissionOK(int addr) { // returns 0=OK, 1=Timeout, 2 = Error during send, 3= NACK during transmit, 4=NACK during complete.
	if (_waitForZeroCross) waitForZeroCross();
	return _wire_port.endTransmission();;
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
	_wire_port.begin(_myAddress);
	_wire_port.setClock(_i2cFreq);
	return true;
}

uint8_t I2C_Talk::getTWIbufferSize() {
	uint8_t junk[1];
	return uint8_t(Wire.write(junk, 100));
}

uint32_t g_timeSince(uint32_t startTime) {
	uint32_t endTime = micros();
	if (endTime < startTime) // Roll-over to zero has occurred - approx every 70 mins.
		return (uint32_t)-1 - startTime + endTime; // +1 to be strictly accurate - but who cares?
	else return endTime - startTime;
}