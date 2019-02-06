#include "TimeLib.h"

namespace Time {

int secondsSinceLastCheck(unsigned long & lastTimeMillis) {
	unsigned long elapsedTime = millis() - lastTimeMillis;
	int _seconds = 0L;
	while (elapsedTime >= 1000L) {
		lastTimeMillis += 1000L;
		elapsedTime -= 1000L;
		++_seconds;
	}
	return _seconds;
}

}
