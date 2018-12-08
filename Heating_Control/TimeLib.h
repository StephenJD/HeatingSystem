#pragma once
#include <Arduino.h>

namespace Time {

int secondsSinceLastCheck(unsigned long & lastTimeMillis); // moves lastTime to last whole second
//inline unsigned long microsSince(unsigned long startMicros) {return micros() - startMicros;} // Overflow works cos using unsigned
//inline unsigned long millisSince(unsigned long startMillis) {return millis() - startMillis;} // Overflow works cos using unsigned
}
