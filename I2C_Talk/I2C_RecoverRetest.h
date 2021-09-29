#pragma once
#include <I2C_Recover.h>
#include <I2C_SpeedTest.h>
#include <Arduino.h>
#include <I2C_RecoverStrategy.h>

class I_I2Cdevice;

namespace I2C_Recovery {

	class I2C_Recover_Retest : public I2C_Recover {
	public:
		using I2C_Recover::I2C_Recover;
		I2C_Recover_Retest(I2C_Talk & i2C, int strategyEEPROMaddr) : I2C_Recover(i2C), _strategy(strategyEEPROMaddr) {}
		static constexpr decltype(millis()) REPEAT_FAILURE_PERIOD = 10000;
		static constexpr decltype(micros()) TIMEOUT = 20000;

		// Definitions for custom reset functions
		//using TestFnPtr = (*)(I2C_Talk &, int)->uint8_t;
		typedef  I2C_Talk_ErrorCodes::Error_codes(*TestFnPtr)(I2C_Talk &, int);
		class I_I2CresetFunctor {
		public:
			virtual I2C_Talk_ErrorCodes::Error_codes operator()(I2C_Talk & i2c, int addr) = 0;
			virtual void postResetInitialisation() {};
		};

		class I2Creset_Functor : public I_I2CresetFunctor {
		public:
			void set(TestFnPtr tfn) { _tfn = tfn; }
			I2C_Talk_ErrorCodes::Error_codes operator()(I2C_Talk & i2c, int addr) override { return _tfn(i2c, addr); }
		private:
			TestFnPtr _tfn = 0;
		};

		// Polymorphic Modifier Functions for I2C_Talk
		auto newReadWrite(I_I2Cdevice_Recovery & device, int retries)->I2C_Talk_ErrorCodes::Error_codes override;
		bool tryReadWriteAgain(I2C_Talk_ErrorCodes::Error_codes status) override;
		void resetI2C() override { call_timeOutFn(device().getAddress()); }
		// Queries 
		I_I2CresetFunctor * getTimeoutFn() const { return _timeoutFunctor; }
		bool isRecovering() { return _isRecovering; }
		bool isUnrecoverable() const override {	return _deviceWaitingOnFailureFor10Mins < 0; }
		// Modifiers for I2C_Recover_Retest
		auto testDevice(int noOfTests, int allowableFailures)-> I2C_Talk_ErrorCodes::Error_codes override;
		void setTimeoutFn(I_I2CresetFunctor * timeoutFnPtr);
		void setTimeoutFn(TestFnPtr timeoutFnPtr);
		void basicTestsBeforeScan(I_I2Cdevice_Recovery & device);
		I2C_RecoverStrategy & strategy() { return _strategy; }
	private:
		// Strategies
		void call_timeOutFn(int addr);
		void endReadWrite();

		// Queries for I2C_Recover_Retest
		// Modifiers for I2C_Recover_Retest
		bool restart(const char * name);
		bool slowdown(); // called by timeoutFunction. Reduces I2C speed by 10% if last called within one second. Returns true if speed was reduced
		void disable() { device().disable(); }
		
		static int _deviceWaitingOnFailureFor10Mins;
		unsigned long _lastRestartTime = 0;
		I2Creset_Functor _i2CresetFunctor; // data member functor to wrap free reset function
		I_I2CresetFunctor * _timeoutFunctor = 0;
		I2C_RecoverStrategy _strategy;
		bool _isRecovering = false;
		int _retries = 0;
	};

}