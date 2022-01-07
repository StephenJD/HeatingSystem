#include "I2C_RecoverStrategy.h"
#include <I2C_Talk.h> // for debug #defines
#include <I2C_Talk_ErrorCodes.h>
#include <EEPROM_RE.h>
#include <FlashStrings.h>

EEPROMClassRE & eeprom();

#ifdef DEBUG_RECOVER
#include <Logging.h>
using namespace arduino_logger;
#endif

using namespace I2C_Talk_ErrorCodes;

namespace I2C_Recovery {

	I2C_RecoverStrategy::I2C_RecoverStrategy(int strategyEEPROMaddr) : _strategyEEPROMaddr(strategyEEPROMaddr) {
		//initialise(); // causes it to freeze at startup
	}

	void I2C_RecoverStrategy::initialise() {
		if (_strategyEEPROMaddr >= 0 && score(0) != STRATEGY_VERSION) {
			//checkEEPROM("I2C_RecoverStrategy::initialise()");
#ifdef DEBUG_RECOVER
			logger() << F("\tI2C_RecoverStrategy::initialise() from ") << _strategyEEPROMaddr << L_endl;
#endif
			for (int s = 1; s < S_NoOfStrategies; ++s) score(s, 0);
			score(0, STRATEGY_VERSION);
		}
			// logger() << F("\n\t Reset Strategy Version:"), score(0));
	}

	uint8_t I2C_RecoverStrategy::score(int index) {
		if (_strategyEEPROMaddr == -1) return 0;
		return eeprom().read(_strategyEEPROMaddr + index);
	}

	void I2C_RecoverStrategy::score(int index, uint8_t val) {
		if (_strategyEEPROMaddr >= 0) {
			//checkEEPROM("I2C_RecoverStrategy::score()");
			eeprom().write(_strategyEEPROMaddr + index, val);
		}
	}

	void I2C_RecoverStrategy::next() {
		initialise();
		int currentScore = 256;
		if (_strategy != S_NoProblem) currentScore = score(_strategy);
		int nextStrategy = _strategy;
		/*do */nextStrategy = (nextStrategy + 1) % S_NoOfStrategies;
		//while (nextStrategy != S_NoProblem && score(nextStrategy) > currentScore);

		//if (currentScore > 0) {
		//	int s = 1;
		//  int nextScore = -1;
		//	for (; s < S_NoOfStrategies; ++s) {
		//		int tryScore = score(s);
		//		if (tryScore < currentScore && tryScore > nextScore) {
		//			nextScore = tryScore;
		//			nextStrategy = s;
		//		}
		//	}
		//}
#ifdef DEBUG_RECOVER
		 logger() << F("\tNext Strategy is ") << nextStrategy << F(" Score: ") << score(nextStrategy) << L_endl;
#endif
		_strategy = static_cast<Strategy>(nextStrategy);
	}

	void I2C_RecoverStrategy::succeeded() {
		if (score(_strategy) == 255) {
			for (int s = 1; s < S_NoOfStrategies; ++s) score(s, score(s) / 2);
		}
		score(_strategy, score(_strategy) + 1);
#ifdef DEBUG_RECOVER
		if (strategy() >= S_Disable) {
			 logger() << F("\t*** Failed & Disabled with strategy ") << strategy() << F(" Score: ") << score(_strategy) << F(" *** \n\n");
		}
		else {
			 logger() << F("\tSucceeded with strategy ") << strategy() << F(" Score: ") << score(_strategy) << L_endl;
		}
		logger();
#endif
		reset();
	}

}

