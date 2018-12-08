#pragma once
//#define TEST_PROGSIM
#define LOAD_EEPROM

#define ZPSIM
#define DEBUG_INFO
#define TEMP_SIMULATION
#define NO_TIMELINE
#define LOG_TO_SD

#if defined TEST_PROGSIM
	#undef LOAD_EEPROM
	#undef DEBUG_INFO
	#undef TEMP_SIMULATION
	#define NO_TIMELINE
#endif

#if !defined ZPSIM
	#undef LOAD_EEPROM
	#undef DEBUG_INFO
	#undef TEMP_SIMULATION
	#define NO_TIMELINE
#endif