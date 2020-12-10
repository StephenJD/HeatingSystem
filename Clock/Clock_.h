#pragma once
#include <Clock.h>

#if !defined _REQUIRES_CLOCK_

inline Clock& clock_() {
	static Clock _clock{};
	return _clock;
}
#endif
