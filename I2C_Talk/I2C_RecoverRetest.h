#pragma once
#include <I2C_Recover.h>
#include <I2C_SpeedTest.h>
#include <Arduino.h>
class I_I2Cdevice;

class I2C_Recover_Retest : public I2C_Recover, public I2C_SpeedTest {
public:
	static constexpr decltype(micros()) REPEAT_FAILURE_PERIOD = 10000;
	static constexpr decltype(micros()) DISABLE_PERIOD_ON_FAILURE = 600000;
	static constexpr auto START_SPEED_AFTER_FAILURE = 100000;
	static constexpr decltype(micros()) TIMEOUT = 20000;
	static constexpr uint8_t _I2C_DATA_PIN = 20; 	//_I2C_DATA_PIN(wire_port == Wire ? 20 : 70)

	I2C_Recover_Retest(I2C_Talk & i2C) : I2C_Recover(i2C), I2C_SpeedTest(i2C, *this) {}
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
	void newReadWrite() override;
	bool tryReadWriteAgain(uint8_t status) override;
	void endReadWrite() override;

	// Modifiers for I2C_Recover_Retest
	void setNoOfRetries(uint8_t retries) { maxRetries = retries; }
	void setTimeoutFn(I_I2CresetFunctor * timeoutFnPtr);
	void setTimeoutFn(TestFnPtr timeoutFnPtr);
	void ensureNotFrozen() override;
	uint8_t findAworkingSpeed(I_I2Cdevice & deviceFailTest) override;
	uint8_t testDevice(int noOfTests, int maxNoOfFailures) override;
	enum strategy {S_delay, S_restart, S_SlowDown, S_SpeedTest, S_PowerDown, S_Disable, S_NoOfStrategies };

private:
	// Queries for I2C_Recover_Retest
	bool restart(const char * name) const;
	uint8_t getNoOfRetries() const { return maxRetries; }
	I_I2CresetFunctor * getTimeoutFn() const { return timeoutFunctor; }
	uint8_t notExists();
	// Modifiers for I2C_Recover_Retest
	bool i2C_is_frozen();
	
	void set_I2C_Talk(I2C_Talk & i2C) override {
		I2C_Recover::set_I2C_Talk(i2C);
		I2C_SpeedTest::set_I2C_Talk(i2C);
	}
	
	void strategey1() {	delay(noOfFailures);}
	void strategey2() {	restart("read 0x"); }
	void strategey3() { slowdown(); }
	void strategey4() {	fastest(device()); }
	void strategey5() { ensureNotFrozen(); }
	void strategey6() {	disable(); }

	bool slowdown(); // called by timoutFunction. Reduces I2C speed by 10% if last called within one second. Returns true if speed was reduced
	//signed char adjustSpeedTillItWorksAgain(I_I2Cdevice * deviceFailTest, int32_t increment);
	void disable() { device().set_runSpeed(0); }
	unsigned long _lastRestartTime = micros();
	I2Creset_Functor _i2CresetFunctor; // data member functor to wrap free reset function
	I_I2CresetFunctor * timeoutFunctor = 0;
	int32_t _lastGoodi2cFreq = 0;
	uint8_t noOfFailures = 0;
	uint8_t maxRetries = 5;
	strategy _strategy;
	uint8_t successAfterRetries = 0;
};	

inline I2C_Recover_Retest::strategy & operator++ (I2C_Recover_Retest::strategy & s) {
	s = static_cast<I2C_Recover_Retest::strategy>((s + 1) % I2C_Recover_Retest::S_NoOfStrategies);
	return s;
}