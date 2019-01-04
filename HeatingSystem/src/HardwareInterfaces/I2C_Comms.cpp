#include "I2C_Comms.h"
#include "Logging.h"
#include "A__Constants.h"

namespace HardwareInterfaces {

	/////////////////////////////////////////////////////
	//              I2C Reset Support                  //
	/////////////////////////////////////////////////////

	ResetI2C::ResetI2C(I_IniFunctor & resetFn, I_TestDevices & testDevices) 
		: _postI2CResetInitialisation(&resetFn),
		_testDevices(&testDevices)
		 {}

	uint8_t ResetI2C::operator()(I2C_Helper & i2c, int addr) {
		//Serial.println("ResetI2C::operator()");
		const uint8_t NO_OF_TRIES = 2;
		static bool isInReset = false;
		static unsigned long nextAllowableResetTime = millis() + 600000ul; // resets every 10 mins for failed speed-test
		if (isInReset) return 0;
		bool thisAddressFailedSpeedTest = (i2c.getThisI2CFrequency(addr) == 0);
		if (thisAddressFailedSpeedTest && millis() < nextAllowableResetTime) return 0;
		isInReset = true;
		nextAllowableResetTime = millis() + 600000ul;
		uint8_t hasFailed = 0, iniFailed = 0, count = NO_OF_TRIES;
		I2C_Helper::I_I2CresetFunctor * origFn = i2c.getTimeoutFn();
		i2c.setTimeoutFn(&hardReset);

		do {
			logger().log("ResetI2C... for ", addr, "try:", NO_OF_TRIES - count + 1);
			hardReset(i2c, addr);

			if (addr != 0) {
				I2C_Helper::I_I2Cdevice & device = _testDevices->getDevice(addr);
				if (device.testDevice(i2c, addr)) {
					logger().log(" Re-test Speed for", addr, " Started at: ", i2c.getI2CFrequency());
					hasFailed = speedTestDevice(i2c, addr, device);
					logger().log(" Re-test Speed Done at:", i2c.getThisI2CFrequency(addr), i2c.getError(hasFailed));
				}
				else logger().log(" Re-test was OK");
			}

			if (hardReset.initialisationRequired) {
				iniFailed = (*_postI2CResetInitialisation)();
				hardReset.initialisationRequired = (iniFailed == 0);
			}
		} while ((hasFailed || iniFailed) && --count > 0);

		i2c.setTimeoutFn(origFn);
		isInReset = false;
		return hasFailed;
	}

	uint8_t ResetI2C::speedTestDevice(I2C_Helper & i2c, int addr, I2C_Helper::I_I2Cdevice & device) {
		I2C_Helper::I_I2CresetFunctor * origFn = i2c.getTimeoutFn();
		i2c.setTimeoutFn(&hardReset);
		i2c.result.reset();
		i2c.result.foundDeviceAddr = addr;
		i2c.speedTestS(&device);
		uint8_t failedTest = i2c.result.error;
		i2c.result.reset();
		i2c.setTimeoutFn(origFn);
		return failedTest;
	}

	bool HardReset::i2C_is_released() {
		//Serial.println("HardReset::i2C_is_released()");
		unsigned long downTime = micros() + 20L;
		while (digitalRead(I2C_DATA) == LOW && micros() < downTime);
		return (digitalRead(I2C_DATA) == HIGH);
	}

	uint8_t HardReset::operator()(I2C_Helper & i2c, int addr) {
		uint8_t resetPerformed = 0;
		//Serial.println("HardReset::operator()");
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
			logger().log(" Hard Reset... for ", addr, " took uS:", downTime);
			delayMicroseconds(50000); // delay to light LED
			digitalWrite(RESET_LED_PIN_N, HIGH);
		}
		return resetPerformed;
	}

}