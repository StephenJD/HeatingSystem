#include "I2C_Comms.h"
#include <Logging/Logging.h>
#include "A__Constants.h"

namespace HardwareInterfaces {

	/////////////////////////////////////////////////////
	//              I2C Reset Support                  //
	/////////////////////////////////////////////////////

	ResetI2C::ResetI2C(I_IniFunctor & resetFn, I_TestDevices & testDevices) 
		: _postI2CResetInitialisation(&resetFn),
		_testDevices(&testDevices)
		 {}

	uint8_t ResetI2C::operator()(I2C_Talk & i2c, int addr) {
		//Serial.println("ResetI2C::operator()");
		const uint8_t NO_OF_TRIES = 2;
		static bool isInReset = false;
		//static unsigned long nextAllowableResetTime = millis() + 600000ul; // resets every 10 mins for failed speed-test
		if (isInReset) {
			logger().log("Test: Recursive Reset... for", addr);
			return 0;
		}

		//bool thisAddressFailedSpeedTest = (i2c.getThisI2CFrequency(addr) == 0);
		//if (thisAddressFailedSpeedTest && millis() < nextAllowableResetTime) return 0;
		isInReset = true;
		//nextAllowableResetTime = millis() + 600000ul;
		uint8_t hasFailed = 0, iniFailed = 0, count = NO_OF_TRIES;
		auto origFn = i2c.getTimeoutFn();
		i2c.setTimeoutFn(&hardReset);

		//do {
			logger().log("ResetI2C... for ", addr, "try:", NO_OF_TRIES - count + 1);
			hardReset(i2c, addr);

			//if (addr != 0) {
			//	I2C_Talk::I_I2Cdevice & device = _testDevices->getDevice(addr);
			//	if (device.testDevice(i2c, addr)) {
			//		logger().log(" Re-test Speed for", addr, " Started at: ", i2c.getI2CFrequency());
			//		hasFailed = speedTestDevice(i2c, addr, device);
			//		logger().log(" Re-test Speed Done at:", i2c.getThisI2CFrequency(addr), i2c.getStatusMsg(hasFailed));
			//	}
			//	else logger().log(" Re-test was OK");
			//}

			if (!i2c.isInScanOrSpeedTest() && hardReset.initialisationRequired) {
				iniFailed = (*_postI2CResetInitialisation)();
				hardReset.initialisationRequired = (iniFailed == 0);
			}
		//} while ((hasFailed || iniFailed) && --count > 0);

		i2c.setTimeoutFn(origFn);
		isInReset = false;
		return hasFailed;
	}

	uint8_t ResetI2C::speedTestDevice(I2C_Talk & i2c, int addr, I_I2Cdevice & device) {
		I2C_Talk::I_I2CresetFunctor * origFn = i2c.getTimeoutFn();
		i2c.setTimeoutFn(&hardReset);
		i2c.result.reset();
		i2c.result.foundDeviceAddr = addr;
		i2c.speedTestS(&device);
		uint8_t failedTest = i2c.result.error;
		i2c.result.reset();
		i2c.setTimeoutFn(origFn);
		return failedTest;
	}

	uint8_t HardReset::operator()(I2C_Talk & i2c, int addr) {
		digitalWrite(RESET_LED_PIN_N, LOW);
		initialisationRequired = true;
		digitalWrite(abs(RESET_OUT_PIN), (RESET_OUT_PIN < 0) ? LOW : HIGH);
		delayMicroseconds(128000); // minimum continuous time required for sucess
		digitalWrite(abs(RESET_OUT_PIN), (RESET_OUT_PIN < 0) ? HIGH : LOW);
		delayMicroseconds(2000); // required to allow supply to recover before re-testing
		if (i2c.i2C_is_frozen(addr)) logger().log("*** Reset I2C is stuck at I2c freq:", i2c.getI2CFrequency(), "for addr:",addr);
		else logger().log("Done Reset for addr:", addr);
		timeOfReset_mS = millis();
		digitalWrite(RESET_LED_PIN_N, HIGH);
		return true;
	}

}