#pragma once
#include <Arduino.h>

#ifdef ZPSIM
#define __FlashStringHelper char
inline const char * F(const char * str) { return str; }
#endif

#define F_SPACE F(" ")
#define F_COLON F(":")
#define F_SLASH F("/")
#define F_ZERO F("0")

//extern const __FlashStringHelper * F_SPACE;
//extern const __FlashStringHelper * F_COLON;
//extern const __FlashStringHelper * F_SLASH;
//extern const __FlashStringHelper * F_ZERO;