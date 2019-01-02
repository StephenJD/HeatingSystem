#include "I2C_Helper.h"

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

int8_t I2C_Helper::s_zeroCrossPin;
uint16_t I2C_Helper::s_zxSigToXdelay; // microseconds

I2C_Helper * I2C_Helper::I_I2Cdevice::_i2C = 0;
int8_t I2C_Helper::TWI_BUFFER_SIZE = 32;

// Public I2C_Helper functions
I2C_Helper::I2C_Helper(TwoWire &wire_port, int32_t i2cFreq)
	: successAfterRetries(0),
	wire_port(wire_port),
	noOfRetries(5),
	timeoutFunctor(0),
	relayDelay(0),
	relayStart(0),
	_myAddress(_single_master),
	_canWrite(true),
	_useAutoSpeed(false) {
	//Serial.println("Create I2C_Helper - singleMaster");
	TWI_BUFFER_SIZE = getTWIbufferSize();
	restart();
	setI2CFrequency(i2cFreq);
	s_zeroCrossPin = 0;
	s_zxSigToXdelay = 700;
	_lastRestartTime = micros();
	}
	
I2C_Helper::I2C_Helper(int multiMaster_MyAddress, TwoWire &wire_port, int32_t i2cFreq) 
	: successAfterRetries(0),
	wire_port(wire_port),
	noOfRetries(5),
	timeoutFunctor(0),
	relayDelay(0),
	relayStart(0),
	_myAddress(multiMaster_MyAddress),
	_canWrite(true),
	_useAutoSpeed(false) {
	//Serial.println("Create I2C_Helper - multiMaster");
	TWI_BUFFER_SIZE = getTWIbufferSize();
	restart();
	setI2CFrequency(i2cFreq);
	s_zeroCrossPin = 0;
	s_zxSigToXdelay = 700;
	_lastRestartTime = micros();
}

I2C_Helper::I2C_Helper(TwoWire &wire_port, int8_t zxPin, uint8_t retries, uint16_t zxDelay, I_I2CresetFunctor * timeOutFn, int32_t i2cFreq)
	: successAfterRetries(0),
	wire_port(wire_port),
	noOfRetries(retries),
	timeoutFunctor(timeOutFn),
	relayDelay(0),
	relayStart(0),
	_myAddress(_single_master),
	_canWrite(true),
	_useAutoSpeed(false)  {
	//log("Construct I2C_Helper - singleMaster");
	TWI_BUFFER_SIZE = getTWIbufferSize();
	restart();
	setI2CFrequency(i2cFreq);
	s_zeroCrossPin = zxPin;
	s_zxSigToXdelay = zxDelay;
	_lastRestartTime = micros();
	}

uint8_t I2C_Helper::notExists(I2C_Helper & i2c, int deviceAddr) {
	return i2c.notExists(deviceAddr);
}

uint8_t I2C_Helper::notExists(int deviceAddr) {
	uint8_t result = beginTransmission(deviceAddr);
	if (result == _OK) {
		//wire_port.write(1);
		return check_endTransmissionOK(deviceAddr);
	} else return result;
}

uint8_t I2C_Helper::read(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, uint8_t *dataBuffer) {
	uint8_t i = 0, returnStatus;
	if (!_canWrite) {
		//Serial.println("I2C_Helper::read slave_shouldnt_write");
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

uint8_t I2C_Helper::readEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, uint8_t *dataBuffer) {
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

uint8_t I2C_Helper::getData(uint16_t deviceAddr, uint16_t numberBytes, uint8_t *dataBuffer) {
	uint8_t returnStatus = check_endTransmissionOK(deviceAddr);
	//retuns 0=_OK, 1=_Insufficient_data_returned, 2=_NACK_during_address_send, 3=_NACK_during_data_send, 4=_NACK_during_complete, 5=_NACK_receiving_data, 6=_Timeout, 7=_slave_shouldnt_write, 8=_I2C_not_created
	if (returnStatus == _OK) {
		returnStatus = (wire_port.requestFrom(deviceAddr, numberBytes) != numberBytes);
		if (returnStatus != _OK) { // returned error
			returnStatus = wire_port.read(); // retrieve error code
			//log("I2C_Helper::read err: for ", long(deviceAddr), getError(returnStatus),long(i));
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

uint8_t I2C_Helper::write(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, const uint8_t * dataBuffer) {
	uint8_t i = 0, returnStatus;
	if (!_canWrite) return _slave_shouldnt_write;
	do{
		returnStatus = beginTransmission(deviceAddr);
		if (returnStatus == _OK) {
			wire_port.write(registerAddress);
			wire_port.write(dataBuffer, numberBytes);
			returnStatus = check_endTransmissionOK(deviceAddr);
			_lastWrite = micros();
			if (returnStatus) {
				//log(" I2C_Helper::write Error Writing addr: ",long(deviceAddr), getError(returnStatus),long(i)); 
				//Serial.print(i,DEC); 
				//Serial.print(deviceAddr,HEX); 
				//Serial.print(" Reg:0x"); 
				//Serial.print(registerAddress,HEX);
				//Serial.println(getError(returnStatus));
			}
		} else return returnStatus;
	} while (returnStatus && ++i < noOfRetries && restart("write 0x",deviceAddr) && g_delayTrue(i));// create an increasing delay if we are going to loop
	if (i < noOfRetries && i > successAfterRetries) successAfterRetries = i;
	return(returnStatus);
}

uint8_t I2C_Helper::write_verify(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer) {
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

uint8_t I2C_Helper::write(uint16_t deviceAddr, uint8_t registerAddress, uint8_t data) {
	return write(deviceAddr, registerAddress, 1, &data);
}

uint8_t I2C_Helper::write(const uint8_t *dataBuffer, uint16_t numberBytes) {
	//Serial.println("Slave Write");
	return (uint8_t) wire_port.write(dataBuffer, numberBytes);
}

uint8_t I2C_Helper::writeEP(uint16_t deviceAddr, int pageAddress, uint8_t data) {
	return writeEP(deviceAddr, pageAddress, 1, &data);
}

uint8_t I2C_Helper::writeEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, const uint8_t *dataBuffer) {
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

void I2C_Helper::waitForDeviceReady(uint16_t deviceAddr)
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

uint8_t I2C_Helper::write_at_zero_cross(uint16_t deviceAddr, uint8_t registerAddress, uint8_t data) {
	uint8_t i = 0, returnStatus;
	do {
		returnStatus = beginTransmission(deviceAddr);
		if (returnStatus == _OK) {
			wire_port.write(registerAddress);
			wire_port.write(data);
			waitForZeroCross();
			returnStatus = check_endTransmissionOK(deviceAddr);
			_lastWrite = micros();
			if (returnStatus) {
				//log(" I2C_Helper::write_at_zero_cross Error Writing addr: ",long(deviceAddr), getError(returnStatus),long(i)); 
				//Serial.print(i,DEC); 
				//Serial.print(" Error Writing@Zero addr:0x"); 
				//Serial.print(deviceAddr,HEX); 
				//Serial.print(" Reg:0x"); 
				//Serial.print(registerAddress,HEX);
				//Serial.println(getError(returnStatus));
			}
			//delay(2);
			relayDelay = g_timeSince(relayStart);	
		} else return returnStatus;
	} while (returnStatus && ++i < noOfRetries && restart("writeZX 0x",deviceAddr) && g_delayTrue(i));// create an increasing delay if we are going to loop
	if (i < noOfRetries && i > successAfterRetries) successAfterRetries = i;
	return(returnStatus);
}

int32_t I2C_Helper::setI2Cfreq_retainAutoSpeed(int32_t i2cFreq) {
	if (_i2cFreq != i2cFreq) {
		_i2cFreq = i2cFreq;
		if (i2cFreq < MIN_I2C_FREQ) _i2cFreq = MIN_I2C_FREQ;
		if (i2cFreq > MAX_I2C_FREQ) _i2cFreq = MAX_I2C_FREQ;
		wire_port.setClock(_i2cFreq);
	}
	return _i2cFreq;
}

int32_t I2C_Helper::setI2CFrequency(int32_t i2cFreq) { // turns auto-speed off
	_useAutoSpeed = false;
	return setI2Cfreq_retainAutoSpeed(i2cFreq);
}

void I2C_Helper::setNoOfRetries(uint8_t retries) {noOfRetries = retries;}
uint8_t I2C_Helper::getNoOfRetries() {return noOfRetries;}
void I2C_Helper::setTimeoutFn (I_I2CresetFunctor * timeOutFn) {timeoutFunctor = timeOutFn;} // Timeout function pointer

void I2C_Helper::setTimeoutFn (TestFnPtr timeOutFn) { // convert function into functor
	_i2CresetFunctor.set(timeOutFn);
	timeoutFunctor = &_i2CresetFunctor;
} // Timeout function pointer

void I2C_Helper::setZeroCross(int8_t zxPin) {s_zeroCrossPin = zxPin;} // Arduino pin signalling zero-cross detected. 0 = disable, +ve = signal on rising edge, -ve = signal on falling edge
void I2C_Helper::setZeroCrossDelay(uint16_t zxDelay) {s_zxSigToXdelay = zxDelay;} // microseconds delay between signal on zero_cross_pin to next true zero-cross.

const char * I2C_Helper::getError(int errorCode) {
 // error_codes {_OK, _Insufficient_data_returned, _NACK_during_address_send, _NACK_during_data_send, _NACK_during_complete, _NACK_receiving_data, _Timeout, _speedError, _slave_shouldnt_write, _I2C_not_created };
	switch (errorCode) {
	case _OK:	return (" No Error");
	case _Insufficient_data_returned:	return (" Insufficient data returned");
	case _NACK_during_address_send:	return (" NACK address send");
	case _NACK_during_data_send:	return (" NACK data send");
	case _NACK_during_complete:	return (" NACK during complete");
	case _NACK_receiving_data:	return (" NACK receiving data");
	case _Timeout:	return (" Timeout");
	case _speedError:	return (" Speed Test Error");
	case _slave_shouldnt_write:	return (" Slave Shouldn't Write");
	case _I2C_not_created:	return (" I2C not created");
	case _I2C_Device_Not_Found: return (" I2C Device not found");
	case _I2C_ReadDataWrong: return (" I2C Read Data Wrong");
	case _I2C_AddressOutOfRange: return (" I2C Address Out of Range");
	default: return (" Not known");
	}
}

uint8_t I2C_Helper::receiveFromMaster(int howMany, uint8_t *dataBuffer) {
	uint8_t noReceived = 0;
	while (wire_port.available() && noReceived < howMany ) {
		dataBuffer[noReceived] = wire_port.read();
		++noReceived;
	}
	return noReceived;
}

// Private Functions
uint8_t I2C_Helper::beginTransmission(uint16_t deviceAddr) { // return false to inhibit access
	if (deviceAddr > 127) return _I2C_AddressOutOfRange;
	if (_useAutoSpeed) {
		if (getThisI2CFrequency(deviceAddr) == 0) return _speedError;
		setI2Cfreq_retainAutoSpeed(getThisI2CFrequency(deviceAddr)); // set device-specific frequency if available
		//Serial.print("Speed for addr: 0x"); Serial.print(deviceAddr, HEX); Serial.print(" Freq: "); Serial.println(getI2CFrequency());
	}
	wire_port.beginTransmission(deviceAddr);
	return _OK;
}

uint8_t I2C_Helper::check_endTransmissionOK(int addr) { // returns 0=OK, 1=Timeout, 2 = Error during send, 3= NACK during transmit, 4=NACK during complete.
	//Serial.println("endTransmission()");
	
	uint8_t error = wire_port.endTransmission();
	if (error == _Timeout) {
		//log("check_endTransmissionOK()_Timeout: calling timeoutFn for: dec",addr);
		callTime_OutFn(addr);
	} 
	//else if (error != _OK) {
		//Serial.print("check_endTransmissionOK() failure: "); Serial.println(getError(error));
	//}
	return error;
}

void I2C_Helper::waitForZeroCross(){
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
#endif
}

bool I2C_Helper::restart() {
	wire_port.begin(_myAddress);
	return true;
}

bool I2C_Helper::restart(const char * name,int addr) {
	restart();
	//Serial.print("I2C::restart() "); Serial.print(name);Serial.println(addr, HEX);
	return true;
}

bool I2C_Helper::slowdown_and_reset(int addr) { // called by timoutFunction
	bool canReduce = false;
	if (micros() - _lastRestartTime < 10000000) { // within 10secs try reducing speed.
		canReduce = getThisI2CFrequency(addr) > MIN_I2C_FREQ;
		if (!canReduce) {
			setThisI2CFrequency(addr, _lastGoodi2cFreq);
		} else {
			setThisI2CFrequency(addr, getI2CFrequency() - getI2CFrequency()/10);
		}
		Serial.print("slowdown_and_reset() New speed: "); Serial.println(_i2cFreq,DEC);
	} else {
		_lastGoodi2cFreq = _i2cFreq;
		setThisI2CFrequency(addr, getI2CFrequency() + getI2CFrequency()/10); // speed up again.
	}
	_lastRestartTime = micros();
	restart();
	return canReduce;
}

uint8_t I2C_Helper::getTWIbufferSize() {
	uint8_t junk[1];
	return wire_port.write(junk, 100);
}

// I2C Testing //
I2C_Helper::scanResult::scanResult() {reset();}

void I2C_Helper::scanResult::reset(){
	foundDeviceAddr = -1;
	totalDevicesFound = 0;
    error = _OK;
    maxSafeSpeed = 0;
	minSafeSpeed = 0;
}

void I2C_Helper::scanResult::prepareNextTest() {
	error = _OK;
	if (maxSafeSpeed == 0) {
		maxSafeSpeed = MAX_I2C_FREQ;
		thisHighestFreq = MIN_I2C_FREQ;
		
	} else thisHighestFreq = maxSafeSpeed;
	if (minSafeSpeed == 0) {
		minSafeSpeed = MIN_I2C_FREQ;
		thisLowestFreq = MAX_I2C_FREQ;
	} else thisLowestFreq = minSafeSpeed;
}

template<>
bool I2C_Helper::scan<false,false>() { // returns false when no more found
	if (!_canWrite) return false;
	useAutoSpeed(false);
	if (result.foundDeviceAddr == -1) {
		result.totalDevicesFound = 0;
	}
	//callTime_OutFn(0);
	while(++result.foundDeviceAddr >= 0) {
		result.error = findAworkingSpeed(0);
		if (result.error == _OK) {
		  ++result.totalDevicesFound;
		  break;
		} else if (result.error == _NACK_during_complete) break; // unknown bus error
	}
	if (result.error == _NACK_during_address_send) result.error = _OK; // don't report expected errors whilst searching for a device
	useAutoSpeed(true);
	return (result.foundDeviceAddr >= 0);
}

void I2C_Helper::callTime_OutFn(int addr) {
	if (timeoutFunctor != 0) {
		//Serial.println("call Time_OutFn");
		(*timeoutFunctor)(*this, addr);
	} else Serial.println("... no Time_OutFn set");
	restart(); // restart i2c in case this is called after an i2c failure
	delayMicroseconds(5000);
}

template<> // implementation of fully specialized template to test one device
uint32_t I2C_Helper::speedTest_T<false,false>(I_I2Cdevice * deviceFailTest) {
	useAutoSpeed(false);
	result.prepareNextTest();
	uint8_t restoreNoOfRetries = getNoOfRetries();
	setNoOfRetries(0);
	uint8_t tryAgain = 0;
	signed char hasFailed = findAworkingSpeed(0);
	if (!hasFailed) {
		//Serial.println(" ** findMaxSpeed **");
		hasFailed = findOptimumSpeed(deviceFailTest, result.thisHighestFreq, MAX_I2C_FREQ /*result.maxSafeSpeed*/);
		//Serial.println(" ** findMinSpeed **");
		hasFailed = hasFailed | findOptimumSpeed(deviceFailTest, result.thisLowestFreq, MIN_I2C_FREQ /*result.minSafeSpeed*/);
	}

	if (hasFailed) {
		result.error = hasFailed; // speed fail
		setThisI2CFrequency(result.foundDeviceAddr, 0); // inhibit future access
	} else {
		setThisI2CFrequency(result.foundDeviceAddr, result.thisHighestFreq);
		//setThisI2CFrequency(result.foundDeviceAddr, (result.thisHighestFreq + result.thisLowestFreq) / 2);
		if (result.thisHighestFreq < result.maxSafeSpeed) result.maxSafeSpeed = result.thisHighestFreq;
		if (result.thisLowestFreq > result.minSafeSpeed) result.minSafeSpeed = result.thisLowestFreq; 
	}
	setNoOfRetries(restoreNoOfRetries);
	useAutoSpeed(true);
	return result.thisHighestFreq;
}

signed char I2C_Helper::testDevice(I_I2Cdevice * deviceFailTest, uint8_t addr, int noOfTestsMustPass) {
	signed char testFailed = 0;
	signed char failed;
	int noOfFailedTests = 0;
	do {
		if (deviceFailTest == 0) {
			//Serial.println("testDevice - none set, using notExists");
			failed = notExists(*this, addr);
		}
		else {
			failed = deviceFailTest->testDevice(*this, addr);
		}
		if (failed) {
			++noOfFailedTests;
			testFailed |= failed;
		}
		--noOfTestsMustPass;
	} while (noOfFailedTests < 2 && (!testFailed && noOfTestsMustPass > 0) || (testFailed && noOfTestsMustPass >= 0));
	if (noOfFailedTests < 2) testFailed = _OK;
	return testFailed;
}

signed char I2C_Helper::findAworkingSpeed(I_I2Cdevice * deviceFailTest) {
	// Must test MAX_I2C_FREQ and MIN_I2C_FREQ as these are skipped in later tests
	uint32_t tryFreq[] = { 52000,8200,330000,3200,21000,1300000,830000,2000,5100,13000,33000,83000,210000,520000,1300000 };
	result.thisHighestFreq = setI2Cfreq_retainAutoSpeed(MAX_I2C_FREQ/2);
	int highFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 1);
	if (highFailed == _I2C_AddressOutOfRange) return highFailed;
	result.thisLowestFreq = setI2Cfreq_retainAutoSpeed(MIN_I2C_FREQ*2);
	int lowFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 1);
	int testFailed = highFailed || lowFailed;
	if (testFailed) {
		for (auto freq : tryFreq) {
			setI2Cfreq_retainAutoSpeed(freq);
			testFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 1);
			//Serial.print("Trying addr: 0x"); Serial.print(result.foundDeviceAddr, HEX); Serial.print(" at freq: "); Serial.println(freq, DEC);
			if (testFailed == _OK) 	break;
			else if (testFailed == _Timeout) callTime_OutFn(0);
		}
	}
	_lastGoodi2cFreq = getI2CFrequency();
	if (highFailed) result.thisHighestFreq = _lastGoodi2cFreq;
	if (lowFailed) result.thisLowestFreq = _lastGoodi2cFreq;
	return testFailed;
}

signed char I2C_Helper::findOptimumSpeed(I_I2Cdevice * deviceFailTest, int32_t & bestSpeed, int32_t limitSpeed ) {
	// enters function at a working frequency
	//callTime_OutFn(0);
	int32_t adjustBy = (limitSpeed - bestSpeed) /3;
	//Serial.print("\n ** findOptimumSpeed start:"); Serial.print(bestSpeed, DEC); Serial.print(" limit: "); Serial.print(limitSpeed, DEC); Serial.print(" adjustBy: "); Serial.println(adjustBy, DEC);
	setI2Cfreq_retainAutoSpeed(limitSpeed);
	signed char hasFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 5); 
	if (hasFailed) {
		bestSpeed = setI2Cfreq_retainAutoSpeed(limitSpeed - adjustBy);
		hasFailed = 0;
		do {
			//Serial.print("\n Try best speed: "); Serial.print(bestSpeed, DEC); Serial.print(" adjustBy : "); Serial.println(adjustBy, DEC);
			while (!hasFailed && (adjustBy > 0 ? bestSpeed < limitSpeed : bestSpeed > limitSpeed)) { // adjust speed 'till it breaks	
				//Serial.print(" Try at: "); Serial.println(getI2CFrequency(), DEC);
				hasFailed = testDevice(deviceFailTest, result.foundDeviceAddr, 2);
				if (hasFailed) {
					limitSpeed = bestSpeed - adjustBy / 100;
					//Serial.print("\n Failed. NewLimit:"); Serial.println(limitSpeed, DEC);
					//callTime_OutFn(0);
					if (hasFailed == _Timeout) callTime_OutFn(0);
				}
				else {
					//Serial.print(" OK. BestSpeed was:"); Serial.println(bestSpeed, DEC);
					bestSpeed = setI2Cfreq_retainAutoSpeed(bestSpeed + adjustBy);
				}
			}
			bestSpeed = setI2Cfreq_retainAutoSpeed(bestSpeed - adjustBy);
			hasFailed = 0;
			adjustBy /= 3;
		} while (abs(adjustBy) > bestSpeed / 20);
		if (hasFailed == _Timeout) callTime_OutFn(0);
		hasFailed = adjustSpeedTillItWorksAgain(deviceFailTest, (adjustBy > 0 ? -10 : 10));
	}
	bestSpeed = getI2CFrequency();
	return hasFailed;
}

signed char I2C_Helper::adjustSpeedTillItWorksAgain(I_I2Cdevice * deviceFailTest, int32_t incrementRatio) {
	constexpr int noOfTestsMustPass = 5;
	signed char hasFailed; // success == 0
	//Serial.print("\n ** Adjust I2C_Speed: "); Serial.println(getI2CFrequency(),DEC);
	_lastGoodi2cFreq = result.thisLowestFreq;
	do { // Adjust speed 'till it works reliably
		hasFailed = testDevice(deviceFailTest,result.foundDeviceAddr, noOfTestsMustPass);
		if (hasFailed) {
			setI2Cfreq_retainAutoSpeed(getI2CFrequency() + (getI2CFrequency() / incrementRatio));
			//Serial.print(" Adjust I2C_Speed: "); Serial.print(getI2CFrequency(),DEC); Serial.print(" increment :"); Serial.println(getI2CFrequency() / incrementRatio,DEC);
			//callTime_OutFn(0);
			if (hasFailed == _Timeout) callTime_OutFn(0);
		}
	} while (hasFailed && getI2CFrequency() > MIN_I2C_FREQ && getI2CFrequency() < MAX_I2C_FREQ);
	//Serial.print(" Adjust I2C_Speed finished at: "); Serial.println(getI2CFrequency(),DEC);
	return hasFailed;
}

// *****************************************************************************************
// ***************************   I2C_Helper_Auto_Speed_Hoist  ******************************
// *****************************************************************************************

int32_t I2C_Helper_Auto_Speed_Hoist::_setI2CFrequency(int8_t devAddr, int32_t i2cFreq, int8_t * devAddrArr, int32_t * i2c_speedArr, int noOfDevices) {
	int index = 0;
	do {
		if (devAddrArr[index] == 0) {
			devAddrArr[index] = devAddr;
		}
		if (devAddrArr[index] == devAddr) {
			i2c_speedArr[index] = i2cFreq;
			break;
		} else {++index;}
	} while (index < noOfDevices) ;
	return setI2Cfreq_retainAutoSpeed(i2cFreq);
}

int32_t I2C_Helper_Auto_Speed_Hoist::_getI2CFrequency(int8_t devAddr, const int8_t * devAddrArr, const int32_t * i2c_speedArr, int noOfDevices) const {
	int index = 0;
	do {
		if (devAddrArr[index] == devAddr) {
			return i2c_speedArr[index];
		} else if (devAddrArr[index] == 0) {
			break;
		} else {++index;}
	} while (index < noOfDevices) ;
	return getI2CFrequency();
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

