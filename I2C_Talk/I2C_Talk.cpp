#include "I2C_Talk.h"
//#include <Logging.h>

#ifndef VARIANT_MCK
#define VARIANT_MCK F_CPU
#endif

using namespace I2C_Talk_ErrorCodes;
using namespace I2C_Recovery;

// encapsulation is improved by using global vars and functions in .cpp file
// rather than declaring these as class statics in the header file,
// since any changes to these would "dirty" the header file unnecessarily.

// private global variables //
static const uint16_t HALF_MAINS_PERIOD = 10000; // in microseconds. 10000 for 50Hz, 8333 for 60Hz

uint32_t g_timeSince(uint32_t startTime);

int8_t I2C_Talk::TWI_BUFFER_SIZE = 32;

void I2C_Talk::setWire(TwoWire & wire_port) { 
	_wire_port = &wire_port;		
	if (ptrdiff_t(&_wire_port) != ptrdiff_t(&Wire)) { _I2C_DATA_PIN = 70; _I2C_CLOCK_PIN = 71; }
}

error_codes I2C_Talk::read(int deviceAddr, int registerAddress, int numberBytes, uint8_t *dataBuffer) {
	//Serial.print("I2C_Talk::read from: 0x");
	//Serial.println(deviceAddr, HEX); Serial.flush();
	//Serial.print(" Reg: ");
	//Serial.print(registerAddress);
	//Serial.print(" Is Wire");
	//Serial.println((long)&_wire_port == (long)&Wire ? "0" : "1");
	auto returnStatus = beginTransmission(deviceAddr);
	if (returnStatus == _OK) {
		_wire().write(registerAddress);
		returnStatus = endTransmission();
		if (returnStatus == _OK) returnStatus = getData(deviceAddr, numberBytes, dataBuffer);
		//Serial.println(getStatusMsg(returnStatus)); Serial.flush();
	}
	return returnStatus;
}

error_codes I2C_Talk::readEP(int deviceAddr, int pageAddress, int numberBytes, uint8_t *dataBuffer) {
	//Serial.print("\treadEP Wire:"); Serial.print((long)&_wire_port); Serial.print(", addr:"); Serial.print(deviceAddr); Serial.print(", page:"); Serial.print(pageAddress);
	//Serial.print(", NoOfBytes:"); Serial.println(numberBytes);
	auto returnStatus = _OK;
	waitForEPready(deviceAddr);
	// NOTE: this puts it in slave mode. Must re-begin to send more data.
	while (numberBytes > 0) {
		uint8_t bytesOnThisPage = min(numberBytes, TWI_BUFFER_SIZE);
		beginTransmission(deviceAddr);
		if (returnStatus == _OK) {
			_wire().write(pageAddress >> 8);   // Address MSB
			_wire().write(pageAddress & 0xff); // Address LSB
			returnStatus = endTransmission();
			if (returnStatus == _OK) returnStatus = getData(deviceAddr, bytesOnThisPage, dataBuffer);
			//Serial.print("ReadEP: status"); Serial.print(getStatusMsg(returnStatus)); Serial.print(" Bytes to read: "); Serial.println(bytesOnThisPage);
		}
		else return returnStatus;
		pageAddress += bytesOnThisPage;
		dataBuffer += bytesOnThisPage;
		numberBytes -= bytesOnThisPage;
	}
	return returnStatus;
}

error_codes I2C_Talk::getData(int deviceAddr, int numberBytes, uint8_t *dataBuffer) {
	// Register address must be loaded into write buffer before entry...
	//retuns 0=_OK, 1=_Insufficient_data_returned, 2=_NACK_during_address_send, 3=_NACK_during_data_send, 4=_NACK_during_complete, 5=_NACK_receiving_data, 6=_Timeout, 7=_slave_shouldnt_write, 8=_I2C_not_created

	uint8_t returnStatus = (_wire().requestFrom((int)deviceAddr, (int)numberBytes) != numberBytes);
	if (returnStatus != _OK) { // returned error
		returnStatus = _wire().read(); // retrieve error code
		//log("I2C_Talk::read err: for ", long(deviceAddr), getStatusMsg(returnStatus),long(i));
	}
	else {
		uint8_t i = 0;
		for (; _wire().available(); ++i) {
			dataBuffer[i] = _wire().read();
		}
		//Serial.print(" Talk.getData OK. for 0x"); Serial.print(deviceAddr, HEX);
		//Serial.print(" Req: "); Serial.print(numberBytes);
		//Serial.print(" Got: "); Serial.println(i);
	}

	return static_cast<error_codes>(returnStatus);
}

error_codes I2C_Talk::write(int deviceAddr, int registerAddress, int numberBytes, const uint8_t * dataBuffer) {
	auto returnStatus = beginTransmission(deviceAddr);
	if (returnStatus == _OK) {
		_wire().write(registerAddress);
		_wire().write(dataBuffer, uint8_t(numberBytes));
		synchroniseWrite();
		returnStatus = endTransmission();
		setProcessTime();	
		//if (returnStatus) {
			//log(" I2C_Talk::write Error Writing addr: ",long(deviceAddr), getStatusMsg(returnStatus),long(i)); 
			//Serial.print(i,DEC); 
			//Serial.print(deviceAddr,HEX); 
			//Serial.print(" Reg:0x"); 
			//Serial.print(registerAddress,HEX);
			//Serial.println(getStatusMsg(returnStatus));
		//}
	}
	return returnStatus;
}

error_codes I2C_Talk::write_verify(int deviceAddr, int registerAddress, int numberBytes, const uint8_t *dataBuffer) {
	uint8_t verifyBuffer[32];
	auto status = write(deviceAddr, registerAddress, numberBytes, dataBuffer);
	if (status == _OK) status = read(deviceAddr, registerAddress, numberBytes, verifyBuffer);
	int i = 0;
	while (status == _OK && i < numberBytes) {
		status = (verifyBuffer[i] == dataBuffer[i] ? _OK : _I2C_ReadDataWrong);
		++i;
	}
	return status;
}

error_codes I2C_Talk::writeEP(int deviceAddr, int pageAddress, int numberBytes, const uint8_t * dataBuffer) {
	auto returnStatus = _OK; 
	// Lambda
	auto _writeEP_block = [returnStatus, deviceAddr, this](int pageAddress, uint16_t numberBytes, const uint8_t * dataBuffer) mutable {
		waitForEPready(deviceAddr);
		_wire().beginTransmission((uint8_t)deviceAddr);
		_wire().write(pageAddress >> 8);   // Address MSB
		_wire().write(pageAddress & 0xff); // Address LSB
		_wire().write(dataBuffer, uint8_t(numberBytes));
		returnStatus = endTransmission();
		_lastWrite = micros();
		return returnStatus;
	};

	auto calcBytesOnThisPage = [](int pageAddress, uint16_t numberBytes) {
		uint8_t bytesUntilPageBoundary = I2C_EEPROM_PAGESIZE - pageAddress % I2C_EEPROM_PAGESIZE;
		uint8_t bytesOnThisPage = min(numberBytes, bytesUntilPageBoundary);
		return min(bytesOnThisPage, TWI_BUFFER_SIZE - 2);
	};
	//Serial.print("\twriteEP Wire:"); Serial.print((long)&_wire_port); Serial.print(", addr:"); Serial.print(deviceAddr); Serial.print(", page:"); Serial.print(pageAddress);
	//Serial.print(", NoOfBytes:"); Serial.println(numberBytes); 
	//Serial.print(" BuffSize:"); Serial.print(TWI_BUFFER_SIZE);
	//Serial.print(", PageSize:"); Serial.println(I2C_EEPROM_PAGESIZE);

	while (returnStatus == _OK && numberBytes > 0) {
		uint8_t bytesOnThisPage = calcBytesOnThisPage(pageAddress, numberBytes);
		returnStatus = _writeEP_block(pageAddress, numberBytes, dataBuffer);
		pageAddress += bytesOnThisPage;
		dataBuffer += bytesOnThisPage;
		numberBytes -= bytesOnThisPage;
	}
	return returnStatus;
}

error_codes I2C_Talk::status(int deviceAddr) // Returns in slave mode.
{
	auto status = beginTransmission(deviceAddr);
	if (status == _OK) {
		status = endTransmission();
		// NOTE: this puts it in slave mode. Must re-begin to send more data.
	}
	return status;
}

void I2C_Talk::waitForEPready(int deviceAddr) {
	constexpr uint32_t I2C_WRITEDELAY = 5000;
	// If writing again within 5mS of last write, wait until EEPROM gives ACK again.
	// this is a bit faster than the hardcoded 5 milliSeconds
	while ((micros() - _lastWrite) <= I2C_WRITEDELAY) {
		_wire().beginTransmission((uint8_t)deviceAddr);
		// NOTE: this puts it in slave mode. Must re-begin to send more data.
		if (endTransmission() == _OK) break;
		yield(); // may be defined for co-routines
	}
}

int32_t I2C_Talk::setI2CFrequency(int32_t i2cFreq) {
	if (_i2cFreq != i2cFreq) {
		_i2cFreq = i2cFreq;
		if (i2cFreq < MIN_I2C_FREQ) _i2cFreq = MIN_I2C_FREQ;
		if (i2cFreq > MAX_I2C_FREQ) _i2cFreq = MAX_I2C_FREQ;
		_wire().setClock(_i2cFreq);
	}
	return _i2cFreq;
}

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
	case _I2C_DataHung_Low: return (" I2C Data Line Hung Low");
	case _I2C_ClockHung_Low: return (" I2C Clock Line Hung Low");
	default: return (" Not known");
	}
}

// Slave response
error_codes I2C_Talk::write(const uint8_t *dataBuffer, int numberBytes) {// Called by slave in response to request from a Master. Return errCode.
	return static_cast<error_codes>(_wire().write(dataBuffer, uint8_t(numberBytes)));
} 

uint8_t I2C_Talk::receiveFromMaster(int howMany, uint8_t *dataBuffer) {
	uint8_t noReceived = 0;
	while (_wire().available() && noReceived < howMany ) {
		dataBuffer[noReceived] = _wire().read();
		++noReceived;
	}
	return noReceived;
}

// Private Functions
error_codes I2C_Talk::beginTransmission(int deviceAddr) { // return false to inhibit access
	auto status = validAddressStatus(deviceAddr);
	if (status == _OK) _wire().beginTransmission((uint8_t)deviceAddr); // Puts in Master Mode.
	return status;
}

error_codes I2C_Talk::endTransmission() {
	auto status = static_cast<I2C_Talk_ErrorCodes::error_codes>(_wire().endTransmission());
	if (status == _Timeout) {
		status = unhangSlaves();
		if (status == _OK) status = _Timeout;
	}
	return status;
}

auto I2C_Talk::unhangSlaves()->I2C_Talk_ErrorCodes::error_codes {
	if (!isMaster()) return _OK;
	pinMode(_I2C_DATA_PIN, INPUT);
	pinMode(_I2C_CLOCK_PIN, OUTPUT);
	for (int dataOneCount = 0, i = 0; (dataOneCount < 10) && (i < 50); ++i) {
		digitalWrite(_I2C_CLOCK_PIN, LOW);
		delayMicroseconds(10);
		digitalWrite(_I2C_CLOCK_PIN, HIGH);
		delayMicroseconds(10);
		if (digitalRead(_I2C_DATA_PIN) == HIGH) ++dataOneCount; // 10 consecutive 1's, no zero's
		else dataOneCount = 0;
	}
	return wait_For_I2C_Lines_OK();
}

auto I2C_Talk::wait_For_I2C_Lines_OK() ->I2C_Talk_ErrorCodes::error_codes {
	unsigned long downTime = micros() + 20L;
	pinMode(_I2C_DATA_PIN, INPUT);
	pinMode(_I2C_CLOCK_PIN, INPUT);
	while (digitalRead(_I2C_DATA_PIN) == LOW && micros() < downTime) {
		//logger() << "WaitFor: " << int(downTime - micros()) << L_flush;
	}
	if (digitalRead(_I2C_DATA_PIN) == LOW) return _I2C_DataHung_Low;
	else if (digitalRead(_I2C_CLOCK_PIN) == LOW) return _I2C_ClockHung_Low;
	wireBegin();
	return _OK;
}

bool I2C_Talk::restart() {
	unhangSlaves();
	_wire().begin(_myAddress);
	_wire().setClock(_i2cFreq);
	TWI_BUFFER_SIZE = getTWIbufferSize();
	//delayMicroseconds(5000);
	//Serial.print("\tI2C_Talk::restart() buffSize "); Serial.println(TWI_BUFFER_SIZE); Serial.flush();
	return true;
}

uint8_t I2C_Talk::getTWIbufferSize() {
	uint8_t junk[1];
	_wire().beginTransmission(1);
	return uint8_t(_wire().write(junk, 100));
}

uint32_t g_timeSince(uint32_t startTime) {
	uint32_t endTime = micros();
	if (endTime < startTime) // Roll-over to zero has occurred - approx every 70 mins.
		return (uint32_t)-1 - startTime + endTime; // +1 to be strictly accurate - but who cares?
	else return endTime - startTime;
}

void I2C_Talk_ZX::setProcessTime() {relayDelay = g_timeSince(relayStart); }

void I2C_Talk_ZX::synchroniseWrite() {
#if !defined (ZPSIM)
	if (_waitForZeroCross) {
		if (s_zeroCrossPin) { //////////// Zero Cross Detection function ////////////////////////
			uint32_t fireTime = micros();
			while (digitalRead(abs(s_zeroCrossPin)) == (s_zeroCrossPin > 0) && g_timeSince(fireTime) < HALF_MAINS_PERIOD); // We need non-active state.
			fireTime = micros();
			while (digitalRead(abs(s_zeroCrossPin)) == (s_zeroCrossPin < 0) && g_timeSince(fireTime) < HALF_MAINS_PERIOD); // Now wait for active state.
			fireTime = micros();
			if (s_zxSigToXdelay < relayDelay) fireTime = fireTime + HALF_MAINS_PERIOD;
			fireTime = fireTime + s_zxSigToXdelay - relayDelay;
			while (micros() < fireTime); // wait for fireTime.
			relayStart = micros(); // time relay delay
		}
		_waitForZeroCross = false;
	}
#endif
}