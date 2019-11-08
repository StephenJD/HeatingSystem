#include <Logging.h>
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

int changeInFreeMemory(int & prevFree, const char * msg) {
	auto newFree = freeMemory() + 24;
	auto changeInMemory = newFree - prevFree;
	prevFree = newFree;
	if (changeInMemory < 0) {
		logger() << msg << " ** Reduced Memory ** by: " << changeInMemory << " to " << newFree << L_endl << L_flush;
	} else if (changeInMemory > 0) {
		logger() << msg << " ** Inceased Memory ** by: " << changeInMemory << " to " << newFree << L_endl << L_flush;
	}
	return changeInMemory;
}