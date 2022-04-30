#include "Timer_mS_uS.h"

///////////////////////////////////////////////////
//         Public Global Functions        //
///////////////////////////////////////////////////

int secondsSinceLastCheck(uint32_t & lastCheck_uS) { // static function
	// move forward one second every 1000 milliseconds
	uint32_t elapsedTime = microsSince(lastCheck_uS);
	int seconds = 0;
	while (elapsedTime >= 1000000) {
		lastCheck_uS += 1000000;
		elapsedTime -= 1000000;
		++seconds;
	}
	return seconds;
}