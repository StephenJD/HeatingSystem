#include "I2C_RecoverStrategy.h"
#include <I2C_Talk_ErrorCodes.h>
#include <EEPROM.h>
//#include <Logging.h>
#include <..\Logging\Logging.h>

//uint8_t resetRTC(I2C_Talk & i2c, int);
//extern I2C_Talk rtc;
//bool checkEEPROM(const char * msg);

using namespace I2C_Talk_ErrorCodes;

namespace I2C_Recovery {

	I2C_RecoverStrategy::I2C_RecoverStrategy(int eeprom_addr) : _eeprom_addr(eeprom_addr) {
		//initialise(); // causes it to freeze at startup
	}

	void I2C_RecoverStrategy::initialise() {
		if (score(0) != STRATEGY_VERSION) {
			//checkEEPROM("I2C_RecoverStrategy::initialise()");
			logger().log("\tI2C_RecoverStrategy::initialise() from", _eeprom_addr);
			for (int s = 1; s < S_NoOfStrategies; ++s) score(s, 0);
			score(0, STRATEGY_VERSION);
		}
			//logger().log("\t Reset Strategy Version:", score(0));
	}

	uint8_t I2C_RecoverStrategy::score(int index) {
		//uint8_t dataBuffa;
		return EEPROM.read(_eeprom_addr + index);
	}

	void I2C_RecoverStrategy::score(int index, uint8_t val) {
		//checkEEPROM("I2C_RecoverStrategy::score()");
		EEPROM.write(_eeprom_addr + index, val);
	}

	void I2C_RecoverStrategy::clear_stackTrace() {
		for (int i = 1; i <= STACKTRACE_MESSAGE_NO; ++i) {
			EEPROM.write(STACKTRACE_MESSAGE_LENGTH * i, '\0');
		}
	}

	void I2C_RecoverStrategy::stackTrace(int index, const char * msg) {
		index = index % STACKTRACE_MESSAGE_NO;
		int startAddr = STACKTRACE_MESSAGE_LENGTH * (index+1);
 		auto endMsg = uint16_t(strlen(msg));
		if (endMsg > STACKTRACE_MESSAGE_LENGTH-2) endMsg = STACKTRACE_MESSAGE_LENGTH-2;
		//bool ep_OK = checkEEPROM("I2C_RecoverStrategy::stackTrace()");
		//Serial.print(startAddr, DEC); Serial.print(" Write ST["); Serial.print(index, DEC); Serial.print("] ");
		//Serial.println(msg); Serial.flush();
		EEPROM.write(startAddr, '@');
		EEPROM.writeEP(startAddr + 1, endMsg, msg);
		//if (!ep_OK) {
		//	char failure[] = { " EPFail" };
		//	if (endMsg > STACKTRACE_MESSAGE_LENGTH-2 - sizeof(failure)) endMsg = STACKTRACE_MESSAGE_LENGTH-2 - sizeof(failure);
		//	EEPROM.writeEP(startAddr + endMsg, sizeof(failure), failure);
		//	endMsg += sizeof(failure);
		//}
		EEPROM.write(startAddr + endMsg + 1, '\0');
		++index;
		index = index % STACKTRACE_MESSAGE_NO;
		EEPROM.write(STACKTRACE_MESSAGE_LENGTH * (index + 1) + 1, '\0'); // delete following message

		//char readMsg[STACKTRACE_MESSAGE_LENGTH] = { '\0' };
		//EEPROM.readEP(startAddr, endMsg + 2, readMsg);
		//Serial.print(startAddr, DEC); Serial.print(" Read ST["); Serial.print(index,DEC);Serial.print("] ");
		//Serial.println(readMsg); Serial.flush();
	}

	void I2C_RecoverStrategy::log_stackTrace() {
		if (EEPROM.getStatus() != _OK) {
			Serial.println("EEPROM Status failed");
			//resetRTC(rtc, 0x50);
		}
		int startAddr = STACKTRACE_MESSAGE_LENGTH;
		char msg[STACKTRACE_MESSAGE_LENGTH] = { 0 };
		//char gotTrace = EEPROM.read(startAddr);
		logger().log("\n***** Start Stack-Trace Log *****");
		
		for (int s = 1; s <= STACKTRACE_MESSAGE_NO; ++s) {
			EEPROM.readEP(startAddr, STACKTRACE_MESSAGE_LENGTH, msg);
			logger().log("Stack-Trace at",startAddr, msg+1);
			startAddr += STACKTRACE_MESSAGE_LENGTH;
			if (EEPROM.read(startAddr) != '@') break;
		}
		logger().log("\n***** End Stack-Trace Log *****");
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
		logger().log("Next Strategy is", nextStrategy, "Score:", score(nextStrategy));
		_strategy = static_cast<Strategy>(nextStrategy);
	}

	void I2C_RecoverStrategy::succeeded() {
		if (score(_strategy) == 255) {
			for (int s = 1; s < S_NoOfStrategies; ++s) score(s, score(s) / 2);
		}
		score(_strategy, score(_strategy) + 1);
		if (strategy() == S_Disable) {
			logger().log("Failed & Disabled with strategy", strategy(), "Score:", score(_strategy));
		}
		else {
			logger().log("Succeeded with strategy", strategy(), "Score:", score(_strategy));
		}
		logger().log();
		reset();
	}

}

