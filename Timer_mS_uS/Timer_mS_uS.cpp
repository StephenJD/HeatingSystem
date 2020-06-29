#include "Timer_mS_uS.h"

///////////////////////////////////////////////////
//         Public Global Functions        //
///////////////////////////////////////////////////

int secondsSinceLastCheck(uint32_t & lastCheck_mS) { // static function
	// move forward one second every 1000 milliseconds
	uint32_t elapsedTime = millisSince(lastCheck_mS);
	int seconds = 0;
	while (elapsedTime >= 1000) {
		lastCheck_mS += 1000;
		elapsedTime -= 1000;
		++seconds;
	}
	return seconds;
}