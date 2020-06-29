//#include <Logging.h>
#include <Arduino.h>
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

#ifdef ZPSIM
char * __malloc_heap_start  = 0;
char * __brkval = 0;
#endif

int freeMemory() {
  char top;
  //logger() << "&top: " << long(&top) << " __brkval: " << long(__brkval) << " __malloc_heap_start: " << long(__malloc_heap_start) << L_endl;
  // __malloc_heap_start is top of statics (start of heap) - approximates to Arduino reporting global variables use.
  // __brkval is the top of the heap, if it has been used, otherwise it's zero.

#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
//#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
//  return &top - __brkval;
#else  // __arm__
  return &top - (__brkval == 0 ? __malloc_heap_start : __brkval);
#endif  // __arm__
}
//
//int changeInFreeMemory(int & prevFree, const char * msg) {
//	auto newFree = freeMemory() + 24;
//	auto changeInMemory = newFree - prevFree;
//	prevFree = newFree;
//	if (changeInMemory < 0) {
//		logger() << msg << F(" ** Reduced Memory ** by: ") << changeInMemory << F(" to ") << newFree << L_endl << L_endl;
//	} else if (changeInMemory > 0) {
//		logger() << msg << F(" ** Inceased Memory ** by: ") << changeInMemory << F(" to ") << newFree << L_endl << L_endl;
//	}
//	return changeInMemory;
//}
//
//#include <stddef.h>
