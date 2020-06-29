#pragma once
#include <Arduino.h>

namespace I2C_Recovery {
	constexpr uint8_t STRATEGY_VERSION = 1;
	enum Strategy { S_NoProblem, S_TryAgain, S_Restart, S_WaitForDataLine/*, S_WaitAgain10*/, S_NotFrozen, S_SlowDown, S_SpeedTest, S_PowerDown, S_Disable, S_Unrecoverable, S_NoOfStrategies };

	class I2C_RecoverStrategy
	{
	public:
		explicit I2C_RecoverStrategy(int strategyEEPROMaddr);
		void initialise();
		void next();
		void succeeded();
		Strategy strategy() { return _strategy; }
		uint8_t score() { return score(_strategy); }
		void reset() { _strategy = S_NoProblem; }
		void tryAgain(Strategy strategy) { _strategy = strategy; }
	private:
		uint8_t score(int index);
		void score(int index, uint8_t val);
		int _strategyEEPROMaddr;
		Strategy _strategy = S_NoProblem;
	};
}

