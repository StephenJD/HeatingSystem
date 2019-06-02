#pragma once
#include <I2C_Recover.h>
#include <I2C_SpeedTest.h>
#include <Arduino.h>
class I_I2Cdevice;

class I2C_Recover_Retest : public I2C_Recover {
public:
	I2C_Recover_Retest(I2C_Talk & i2C) : I2C_Recover(i2C) {}
	// Definitions for custom reset functions
	//using TestFnPtr = (*)(I2C_Talk &, int)->uint8_t;
	typedef  uint8_t(*TestFnPtr)(I2C_Talk &, int);
	class I_I2CresetFunctor {
	public:
		virtual uint8_t operator()(I2C_Talk & i2c, int addr) = 0;
	};

	class I2Creset_Functor : public I_I2CresetFunctor {
	public:
		void set(TestFnPtr tfn) { _tfn = tfn; }
		uint8_t operator()(I2C_Talk & i2c, int addr) override { return _tfn(i2c, addr); }
	private:
		TestFnPtr _tfn;
	};

	// Polymorphic Functions for I2C_Talk
	void newReadWrite() override { noOfFailures = 0; }
	bool tryReadWriteAgain(uint8_t status) override;
	void endReadWrite() override;
	bool checkForTimeout(uint8_t status) override;
	uint8_t speedOK() override;
	bool endTransmissionError(uint8_t status) override;

	// Modifiers for I2C_Recover_Retest
	void setNoOfRetries(uint8_t retries) { noOfRetries = retries; }
	void setTimeoutFn(I_I2CresetFunctor * timeoutFnPtr);
	void setTimeoutFn(TestFnPtr timeoutFnPtr);
	void callTime_OutFn() override;
	signed char findAworkingSpeed(I_I2Cdevice * deviceFailTest) override;

private:
	// Queries for I2C_Recover_Retest
	bool restart(const char * name) const;
	uint8_t getNoOfRetries() const { return noOfRetries; }
	I_I2CresetFunctor * getTimeoutFn() const { return timeoutFunctor; }
	// Modifiers for I2C_Recover_Retest
	bool i2C_is_frozen();

	signed char testDevice(I_I2Cdevice * deviceFailTest, int noOfTestsMustPass);

	bool isInScanOrSpeedTest() { return _inScanOrSpeedTest; }
	bool slowdown_and_reset(); // called by timoutFunction. Reduces I2C speed by 10% if last called within one second. Returns true if speed was reduced
	signed char adjustSpeedTillItWorksAgain(I_I2Cdevice * deviceFailTest, int32_t increment);

	unsigned long _lastRestartTime = micros();
	I2C_SpeedTest _speedTest;
	I2Creset_Functor _i2CresetFunctor; // data member functor to wrap free reset function
	I_I2CresetFunctor * timeoutFunctor = 0;
	int32_t _lastGoodi2cFreq = 0;
	uint8_t noOfFailures = 0;
	uint8_t noOfRetries = 5;
	uint8_t successAfterRetries = 0;
	bool _inScanOrSpeedTest = false;

	const uint8_t _I2C_DATA_PIN = 20;
};