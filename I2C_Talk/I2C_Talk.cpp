#include "I2C_Talk.h"
//#include <FlashStrings.h>
// 
//Using this library requires a modified wire.h, wire.cpp(AVR& Sam) and twi.h& twi.c(AVR)
#ifndef MODIFIED_WIRE_LIB
	static_assert(false, "Missing wire mods");
#endif

#ifndef VARIANT_MCK
#define VARIANT_MCK F_CPU
#endif

#if defined DEBUG_TALK || defined SHOW_TWI_DEBUG
#include <Logging.h>
namespace arduino_logger {
	Logger& logger();
}
using namespace arduino_logger;
#endif

#ifdef SHOW_TWI_DEBUG
// For use when debugging twi.c
extern "C" void I2C_Talk_msg(const char * str) {
	logger() << str << L_endl;
}

extern "C" void I2C_Talk_msg2(const char * str, long val1, long val2) {
	logger() << str << val1 << F(" : ") << val2 << L_endl;
}
#endif

using namespace I2C_Talk_ErrorCodes;
using namespace I2C_Recovery;

// encapsulation is improved by using global vars and functions in .cpp file
// rather than declaring these as class statics in the header file,
// since any changes to these would "dirty" the header file unnecessarily.

// private global variables //
constexpr uint16_t HALF_MAINS_PERIOD = 10000; // in microseconds. 10000 for 50Hz, 8333 for 60Hz

bool hasExpired(uint32_t someTime_uS) {
	int32_t timeToGo = someTime_uS - micros();
	return timeToGo <= 0 || timeToGo > 20000;
}

int8_t I2C_Talk::TWI_BUFFER_SIZE = 0;

I2C_Talk::I2C_Talk(int multiMaster_MyAddress, TwoWire & wire_port, int32_t max_I2Cfreq) :
	_max_i2cFreq(max_I2Cfreq)
	, _i2cFreq(max_I2Cfreq)
	, _myAddress(multiMaster_MyAddress)
	, _wire_port(&wire_port)
{}

void I2C_Talk::ini(TwoWire & wire_port, int32_t max_I2Cfreq) {
	_max_i2cFreq = max_I2Cfreq;
	_i2cFreq = max_I2Cfreq;
	_wire_port = &wire_port;
}


Error_codes I2C_Talk::read(int deviceAddr, int registerAddress, int numberBytes, volatile uint8_t *dataBuffer) {
	auto returnStatus = _OK;
	returnStatus = beginTransmission(deviceAddr);
	if (returnStatus == _OK) {
		_wire().write(registerAddress); // just adds reg to send-buffer
#ifdef DEBUG_TALK
		if (deviceAddr == 0x13) logger() << L_time << "Read 0x" << L_hex << deviceAddr << L_dec << " Send RA:" << registerAddress << L_endl;
#endif
		returnStatus = endTransmission();
		returnStatus = getData(returnStatus, deviceAddr, numberBytes, dataBuffer);
	}
	return returnStatus;
}

Error_codes I2C_Talk::readEP(int deviceAddr, int pageAddress, int numberBytes, uint8_t *dataBuffer) {
	//Serial.print("\treadEP Wire:"); Serial.print(reinterpret_cast<uintptr_t>(&_wire_port)); Serial.print(", addr:"); Serial.print(deviceAddr); Serial.print(", page:"); Serial.print(pageAddress);
	//Serial.print(", NoOfBytes:"); Serial.println(numberBytes);
	auto returnStatus = _OK;
	// NOTE: this puts it in slave mode. Must re-begin to send more data.
	while (numberBytes > 0) {
		uint8_t bytesOnThisPage = min(numberBytes, TWI_BUFFER_SIZE);
		returnStatus |= beginTransmission(deviceAddr);
		if (returnStatus == _OK) {
			_wire().write(pageAddress >> 8);   // Address MSB
			_wire().write(pageAddress & 0xff); // Address LSB
			returnStatus = endTransmission();
			returnStatus |= getData(returnStatus, deviceAddr, bytesOnThisPage, dataBuffer);
			//Serial.print("ReadEP: status"); Serial.print(getStatusMsg(returnStatus)); Serial.print(" Bytes to read: "); Serial.println(bytesOnThisPage);
		}
		else return returnStatus;
		pageAddress += bytesOnThisPage;
		dataBuffer += bytesOnThisPage;
		numberBytes -= bytesOnThisPage;
	}
	return returnStatus;
}

Error_codes I2C_Talk::getData(Error_codes status, int deviceAddr, int numberBytes, volatile uint8_t *dataBuffer) {
	// Register address must be loaded into write buffer before entry...
	//retuns 0=_OK, 1=_Insufficient_data_returned, 2=_NACK_during_address_send, 3=_NACK_during_data_send, 4=_NACK_during_complete, 5=_NACK_receiving_data, 6=_Timeout, 7=_slave_shouldnt_write, 8=_I2C_not_created
	//logger() << F(" I2C_Talk::getData start") << L_endl;
	if (status == _OK) {
		auto bytesAvailable = _wire().requestFrom((int)deviceAddr, (int)numberBytes);
		status = Error_codes(bytesAvailable != numberBytes);
	
		if (status == _OK) {
#ifdef DEBUG_TALK
			//logger() << F(" I2C_Talk::getData for: 0x") << L_hex << deviceAddr << F(" NoOfBytes:") << bytesAvailable << L_endl;
#endif
			for (auto i = 0; i < bytesAvailable; ++i) {
				dataBuffer[i] = _wire().read();
			}
		} else { // returned error
			status = Error_codes(_wire().read()); // retrieve error code
#ifdef DEBUG_TALK
			logger() << F(" I2C_Talk::getData Error for: 0x") << L_hex << deviceAddr << getStatusMsg(status) << L_endl;
#endif
		}
	} else {
#ifdef DEBUG_TALK
		logger() << F(" I2C_Talk::read, sendRegister failed for 0x") << L_hex << deviceAddr << getStatusMsg(status) << L_endl;
#endif
	}
	// Leave buffer unchanged if read fails
	//if (status != _OK) {
	//	//Serial.print(F("reqStat: ")); Serial.println(status);
	//	for (auto i = 0; i < numberBytes; ++i) {
	//		dataBuffer[i] = 0;
	//	}
	//}
	return status;
}

Error_codes I2C_Talk::write(int deviceAddr, int registerAddress, int numberBytes, volatile const uint8_t * dataBuffer) {
	//logger() << F(" I2C_Talk::write...") << L_endl;
	auto returnStatus = _OK;
	returnStatus = beginTransmission(deviceAddr);
	if (returnStatus == _OK) {
		_wire().write(registerAddress); // just writes to buffer
		_wire().write(const_cast<const uint8_t*>(dataBuffer), uint8_t(numberBytes));
		synchroniseWrite();
		//logger() << F(" I2C_Talk::write send data..") << L_endl; 
		returnStatus = endTransmission(); // returns address-send or data-send error
		if (isMultiMaster()) _lastWrite = micros(); else _lastWrite = micros() - I2C_MULTI_MASTER_DELAY_uS; // allow time for other masters to write.
		setProcessTime();
	}

#ifdef DEBUG_TALK
	if (returnStatus) {
		logger() << F(" I2C_Talk::write failed for 0x") << L_hex << deviceAddr << getStatusMsg(returnStatus) << L_endl; 
	}
#endif	
	return returnStatus;
}

Error_codes I2C_Talk::write_verify(int deviceAddr, int registerAddress, int numberBytes, volatile const uint8_t *dataBuffer) {
	uint8_t verifyBuffer[32] = {0xAA,0xAA};
	auto status = write(deviceAddr, registerAddress, numberBytes, dataBuffer);
	if (status == _OK) status = read(deviceAddr, registerAddress, numberBytes, verifyBuffer);
	int i = 0;
	while (status == _OK && i < numberBytes) {
		status |= (verifyBuffer[i] == dataBuffer[i] ? _OK : _I2C_ReadDataWrong);
		++i;
	}
	return status;
}

Error_codes I2C_Talk::writeEP(int deviceAddr, int pageAddress, int numberBytes, const uint8_t * dataBuffer) {
	auto returnStatus = _OK; 
	// Lambda
	auto _writeEP_block = [returnStatus, deviceAddr, this](int pageAddress, uint16_t numberBytes, const uint8_t * dataBuffer) mutable {
		beginTransmission((uint8_t)deviceAddr);
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


	while (returnStatus == _OK && numberBytes > 0) {
		uint8_t bytesOnThisPage = calcBytesOnThisPage(pageAddress, numberBytes);
		returnStatus = _writeEP_block(pageAddress, bytesOnThisPage, dataBuffer);
#ifdef DEBUG_TALK
		//logger() << "\twriteEP page: " << pageAddress << ", NoOfBytes:" << bytesOnThisPage << L_endl;
#endif
		pageAddress += bytesOnThisPage;
		dataBuffer += bytesOnThisPage;
		numberBytes -= bytesOnThisPage;
	}
	return returnStatus;
}

void I2C_Talk::setMax_i2cFreq(int32_t max_I2Cfreq) { 
	if (max_I2Cfreq > VARIANT_MCK / 36) max_I2Cfreq = VARIANT_MCK / 36;
	if (max_I2Cfreq < _max_i2cFreq) _max_i2cFreq = max_I2Cfreq;
	// Must NOT setI2CFrequency() as Wire may not yet be constructed.
}

int32_t I2C_Talk::setI2CFrequency(int32_t i2cFreq) {
	if (_i2cFreq != i2cFreq) {
		_i2cFreq = i2cFreq;
		if (i2cFreq < MIN_I2C_FREQ) _i2cFreq = MIN_I2C_FREQ;
		if (i2cFreq > _max_i2cFreq) _i2cFreq = _max_i2cFreq;
		_wire().setClock(_i2cFreq);
	}
	return _i2cFreq;
}

void I2C_Talk::setTimeouts(uint32_t slaveByteProcess_uS, int stopMargin_uS, uint32_t busRelease_uS) {
	_slaveByteProcess_uS = slaveByteProcess_uS;
	_stopMargin_uS = stopMargin_uS;
	_busRelease_uS = busRelease_uS;
	if (_wire_port) {
		_wire().setTimeouts(_slaveByteProcess_uS, _stopMargin_uS, _busRelease_uS);
		// Must NOT setClock() as Wire may not yet be constructed.
	}
}

void I2C_Talk::extendTimeouts(uint32_t slaveByteProcess_uS, int stopMargin_uS, uint32_t busRelease_uS) {
	if (slaveByteProcess_uS > _slaveByteProcess_uS) _slaveByteProcess_uS = slaveByteProcess_uS;
	if (stopMargin_uS > _stopMargin_uS) _stopMargin_uS = stopMargin_uS;
	if (busRelease_uS > _busRelease_uS) _busRelease_uS = busRelease_uS;
	if (_wire_port) {
		_wire().setTimeouts(_slaveByteProcess_uS, _stopMargin_uS, _busRelease_uS);
		// Must NOT setClock() as Wire may not yet be constructed.
	}
}

void I2C_Talk::setStopMargin(uint8_t margin) { 
	_stopMargin_uS = margin;
	if (_wire_port) {
		_wire().setTimeouts(_slaveByteProcess_uS, _stopMargin_uS, _busRelease_uS);
		// Must NOT setClock() as Wire may not yet be constructed.
	}
}

const __FlashStringHelper * I2C_Talk::getStatusMsg(int errorCode) {
	switch (errorCode) {
	case _OK:	return F(" No Error");
	case _Insufficient_data_returned:	return F(" Insufficient data returned");
	case _NACK_during_address_send:	return F(" NACK address send");
	case _NACK_during_data_send:	return F(" NACK data send");
	case _NACK_during_complete:	return F(" NACK during complete");
	case _NACK_receiving_data:	return F(" NACK receiving data");
	case _Timeout:	return F(" Timeout");
	case _StopMarginTimeout: return F(" Stop Margin Timeout");
	case _SlaveByteProcessTimeout: return F(" Slave Byte Process Timeout");
	case _BusReleaseTimeout: return F(" Bus Release Timeout (Data hung low)");
	case _I2C_ClockHungLow: return F(" Clock Hung Low");
	case _disabledDevice:	return F(" Device Disabled. Enable with set_runSpeed()");
	case _slave_shouldnt_write:	return F(" Slave Shouldn't Write");
	case _I2C_Device_Not_Found: return F(" I2C Device not found");
	case _I2C_ReadDataWrong: return F(" I2C Read Data Wrong");
	case _I2C_AddressOutOfRange: return F(" I2C Address Out of Range");
	case _I2C_NotBegun: return F(" I2C NotBegun");
	case 0xFF: return F(" Need modified Wire.cpp/twi.c");
	default: return F(" Not known");
	}
}

// Slave response
Error_codes I2C_Talk::write(volatile const uint8_t* dataBuffer, int numberBytes) {// Called by slave in response to request from a Master. Return errCode.
	// Writes from the databuffer to the I2C comms.
	//logger() << F("Slave-write as ") << (isMaster() ? F("master") : F("slave")) << L_endl;
	return static_cast<Error_codes>(_wire().write((const uint8_t*)dataBuffer, uint8_t(numberBytes)));
} 

uint8_t I2C_Talk::receiveFromMaster(int howMany, volatile uint8_t *dataBuffer) {
	uint8_t noReceived = 0;
	while (_wire().available() && noReceived < howMany ) {
		dataBuffer[noReceived] = _wire().read();
		++noReceived;
	}
	for (int i = noReceived; i < howMany; ++i) dataBuffer[i] = 0;
	return noReceived;
}

Error_codes I2C_Talk::status(int deviceAddr) const // Returns in slave mode.
{
	Error_codes status = beginTransmission(deviceAddr);
	if (status == _OK) {
		status = endTransmission();
	}
	return status;
}

// Private Functions
Error_codes I2C_Talk::beginTransmission(int deviceAddr) const { // return false to inhibit access
	//auto _exec_time = micros();
	//logger() << "beginTrans _lastWrite: " << _exec_time - _lastWrite << L_endl;
	auto status = _wire_port == 0 ? _I2C_NotBegun  : validAddressStatus(deviceAddr);
	if (status == _OK) {
#ifndef ZPSIM
		if (isMultiMaster()) {
			while (!hasExpired(_lastWrite + I2C_MULTI_MASTER_DELAY_uS));
		}
#endif
		//logger() << "beginTrans expir: " << micros() - _exec_time << L_endl; _exec_time = micros();
		while (!hasExpired(_lastWrite + I2C_EEPROM_WRITE_DELAY_uS)) {
			//logger() << "beginTrans WRITE_DELAY: " << micros() - _exec_time << L_endl; _exec_time = micros();
			_wire().beginTransmission((uint8_t)deviceAddr);
			// NOTE: this puts it in slave mode. Must re-begin to send more data.
			status = static_cast<I2C_Talk_ErrorCodes::Error_codes>(_wire().endTransmission());
			if (status == _OK) break;
			yield(); // may be defined for co-routines
		}
		_wire().beginTransmission((uint8_t)deviceAddr); // Puts in Master Mode.
	}
	return status;
}

Error_codes I2C_Talk::endTransmission() const {
	//auto _exec_time = micros();
	//if (isMultiMaster()) while (!hasExpired(_lastWrite + I2C_MULTI_MASTER_DELAY_uS));
	//logger() << "endTrans wait: " << micros() - _exec_time << L_endl; _exec_time = micros();
#ifndef ZPSIM
	while (!hasExpired(_lastWrite + _addressDelay));
#endif
	auto status = static_cast<I2C_Talk_ErrorCodes::Error_codes>(_wire().endTransmission());
	_lastWrite = micros();

#ifdef DEBUG_TALK
	if (status >= _Timeout) {
		logger() << F("_wire().endTransmission() returned ") << status << getStatusMsg(status) << L_endl;
	}
#endif
	return status;
}

bool I2C_Talk::begin() {
	//Flogger() << F("I2C_Talk::begin()") << L_endl;
	if (TWI_BUFFER_SIZE == 0) TWI_BUFFER_SIZE = getTWIbufferSize();
	_lastWrite = micros();
	_wire().begin(_myAddress); 
	--_i2cFreq;  
	setI2CFrequency(_i2cFreq + 1); 
	_wire().setTimeouts(_slaveByteProcess_uS, _stopMargin_uS, _busRelease_uS);
	return true;
}

uint8_t I2C_Talk::getTWIbufferSize() {
	uint8_t junk[1];
	_wire().beginTransmission(1);
#if defined (ZPSIM)
	return 32;
#endif
	return uint8_t(_wire().write(junk, 100));
}

void I2C_Talk_ZX::setProcessTime() {relayDelay = uint16_t(micros() - relayStart); }

void I2C_Talk_ZX::synchroniseWrite() {
#if !defined (ZPSIM)
	if (_waitForZeroCross) {
		if (s_zeroCrossPinWatch.port() != 0) { //////////// Zero Cross Detection function ////////////////////////
			uint32_t zeroCrossSignalTime = micros();
			//logger() << "Wait for zLow" << L_endl;
			while (s_zeroCrossPinWatch.logicalState() != false && !hasExpired(zeroCrossSignalTime + HALF_MAINS_PERIOD)); // We need non-active state.
			zeroCrossSignalTime = micros();
			//logger() << "Wait for zHigh" << L_endl;
			while (s_zeroCrossPinWatch.logicalState() != true && !hasExpired(zeroCrossSignalTime + HALF_MAINS_PERIOD)); // Now wait for active state.
			zeroCrossSignalTime = micros();
			int32_t fireDelay = s_zxSigToXdelay - relayDelay;
			if (fireDelay < 0) fireDelay += HALF_MAINS_PERIOD;
			//logger() << "Delay for zzz " << fireDelay << L_endl;
			while (!hasExpired(zeroCrossSignalTime + fireDelay)) {
				//logger() << "so far... " << int32_t(micros() - zeroCrossSignalTime) << L_endl; // wait for fireTime.
			}
			relayStart = micros(); // time relay delay
		}
		_waitForZeroCross = false;
	}
#endif
}