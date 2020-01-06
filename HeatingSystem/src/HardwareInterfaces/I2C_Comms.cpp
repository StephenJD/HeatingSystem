#include "I2C_Comms.h"
#include <Logging.h>
#include "A__Constants.h"
#include <I2C_Recover.h>

using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;
namespace HardwareInterfaces {

	/////////////////////////////////////////////////////
	//              I2C Reset Support                  //
	/////////////////////////////////////////////////////

	ResetI2C::ResetI2C(I2C_Recover_Retest & recover, I_IniFunctor & resetFn, I_TestDevices & testDevices)
		:
		_recover(&recover)
		, _postI2CResetInitialisation(&resetFn)
		, _testDevices(&testDevices)
		{
			pinMode(RESET_LED_PIN_P, OUTPUT);
			pinMode(RESET_LED_PIN_N, OUTPUT);
			digitalWrite(RESET_LED_PIN_P, HIGH);
			digitalWrite(RESET_LED_PIN_N, HIGH);
			pinMode(abs(RESET_OUT_PIN), OUTPUT);
			digitalWrite(abs(RESET_OUT_PIN), (RESET_OUT_PIN < 0) ? HIGH : LOW);
			_recover->setTimeoutFn(this);
		}

	error_codes ResetI2C::operator()(I2C_Talk & i2c, int addr) {
		static bool isInReset = false;
		if (isInReset) {
			logger() << "\nTest: Recursive Reset... for 0x" << L_hex << addr << L_endl;
			return _OK;
		}

		isInReset = true;
		error_codes status = _OK;
		auto origFn = _recover->getTimeoutFn();
		_recover->setTimeoutFn(&hardReset);

		logger() << "\t\tResetI2C... for 0x" << L_hex << addr << L_endl;
		hardReset(i2c, addr);
		if (!_recover->isRecovering()) {
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
			logger() << "\t\tResetI2C... _postI2CResetInitialisation\n";
			if ((*_postI2CResetInitialisation)() != 0) return; // return 0 for OK. Resets hardReset.initialisationRequired to false.
		}
	};


	error_codes HardReset::operator()(I2C_Talk & i2c, int addr) {
		digitalWrite(RESET_LED_PIN_N, LOW);
		digitalWrite(abs(RESET_OUT_PIN), (RESET_OUT_PIN < 0) ? LOW : HIGH);
		delayMicroseconds(128000); // minimum continuous time required for sucess
		digitalWrite(abs(RESET_OUT_PIN), (RESET_OUT_PIN < 0) ? HIGH : LOW);
		//delayMicroseconds(2000); // required to allow supply to recover before re-testing
		i2c.restart();
		timeOfReset_mS = millis();
		//if (i2c.i2C_is_frozen(addr)) logger() << "*** Reset I2C is stuck at I2c freq:", i2c.getI2CFrequency(), "for addr:",addr);
		//else 
			logger() << L_time << "Done Hard Reset for 0x" << L_hex << addr << L_endl;
		digitalWrite(RESET_LED_PIN_N, HIGH);
		initialisationRequired = true;
		return _OK;
	}

}