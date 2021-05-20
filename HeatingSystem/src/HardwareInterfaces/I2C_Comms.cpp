#include "I2C_Comms.h"
#include <Logging.h>
#include "A__Constants.h"
#include <I2C_Recover.h>
#include <LocalKeypad.h>

using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;
using namespace arduino_logger;

namespace HardwareInterfaces {

	/////////////////////////////////////////////////////
	//              I2C Reset Support                  //
	/////////////////////////////////////////////////////

	ResetI2C::ResetI2C(I2C_Recover_Retest & recover, I_IniFunctor & resetFn, I_TestDevices & testDevices, Pin_Wag resetPinWag)
		:
		_recover(&recover)
		, hardReset(resetPinWag)
		, _postI2CResetInitialisation(&resetFn)
		, _testDevices(&testDevices)
		{
			_recover->setTimeoutFn(this);
			_recover->i2C().begin();
			_recover->i2C().setTimeouts(15000, I2C_Talk::WORKING_STOP_TIMEOUT, 5000); // give generous stop-timeout in normal use 
		}

	Error_codes ResetI2C::operator()(I2C_Talk & i2c, int addr) {
		static bool isInReset = false;
		if (isInReset) {
			logger() << F("\nTest: Recursive Reset... for 0x") << L_hex << addr << L_endl;
			return _OK;
		}

		isInReset = true;
		Error_codes status = _OK;
		auto origFn = _recover->getTimeoutFn();
		_recover->setTimeoutFn(&hardReset);

		logger() << F("\t\tResetI2C... for 0x") << L_hex << addr << L_endl;
		hardReset(i2c, addr);
		if (!_recover->isRecovering()) {
			logger() << "\tResetI2C Doing retest" << L_endl;
			I_I2Cdevice_Recovery & device = _testDevices->getDevice(addr);
			status = device.testDevice();
			if (status == _OK) postResetInitialisation();
		}

		_recover->setTimeoutFn(origFn);
		isInReset = false;
		return status;
	}

	void ResetI2C::postResetInitialisation() { 
		if (hardReset.initialisationRequired && _postI2CResetInitialisation) {
			logger() << F("\t\tResetI2C... _postI2CResetInitialisation\n");
			if ((*_postI2CResetInitialisation)() != 0) return; // return 0 for OK. Resets hardReset.initialisationRequired to false.
		}
	};

	void HardReset::arduinoReset(const char * msg) {
		logger() << L_flush << F("\n *** HardReset::arduinoReset called by ") << msg << L_endl << L_flush;
		pinMode(RESET_5vREF_PIN, OUTPUT);
		digitalWrite(RESET_5vREF_PIN, LOW);
	}

	Error_codes HardReset::operator()(I2C_Talk & i2c, int addr) {
		LocalKeypad::indicatorLED().set();
		_resetPinWag.set();
		delay(128); // interrupts still serviced
		delay(128); 
		delay(128); 
		delay(128); 
		_resetPinWag.clear();
		i2c.begin();
		timeOfReset_mS = millis();
		if (timeOfReset_mS == 0) timeOfReset_mS = 1;
		logger() << L_time << F("Done Hard Reset for 0x") << L_hex << addr << L_endl;
		LocalKeypad::indicatorLED().clear();
		if (digitalRead(20) == LOW)
			logger() << "\tData stuck after reset" << L_endl;
		initialisationRequired = true;
		return _OK;
	}

	namespace I2C_Slave {
		I2C_Talk* i2C;
		uint8_t reg = 0;
		int8_t data = -1;

		// Called when data is sent by Master, telling slave how many bytes have been sent.
		void receiveI2C(int howMany) {
			// must not do a Serial.print when it receives a register address prior to a read.
			uint8_t msgFromMaster[3]; // = [register, data-0, data-1]
			if (howMany > 3) howMany = 3;
			int noOfBytesReceived = i2C->receiveFromMaster(howMany, msgFromMaster);
			if (noOfBytesReceived) {
				reg = msgFromMaster[0];
				if (howMany > 1) data = msgFromMaster[1];
			}
		}

		// Called when data is requested.
		// The Master will send a NACK when it has received the requested number of bytes, so we should offer as many as could be requested.
		// To keep things simple, we will send a max of two-bytes.
		// Arduino uses little endianness: LSB at the smallest address: So a uint16_t is [LSB, MSB].
		// But I2C devices use big-endianness: MSB at the smallest address: So a uint16_t is [MSB, LSB].
		// A single read returns the MSB of a 2-byte value (as in a Temp Sensor), 2-byte read is required to get the LSB.
		// To make single-byte read-write work, all registers are treated as two-byte, with second-byte un-used for most. 
		// getRegister() returns uint8_t as uint16_t which puts LSB at the lower address, which is what we want!
		// We can send the address of the returned value to i2C().write().
		// 
		void requestI2C() {
			i2C->write((uint8_t*)&data, 1);
		}

		int getKey() { 
			auto key = data; 
			data = -1; 
			return key; 
		}
	}
}