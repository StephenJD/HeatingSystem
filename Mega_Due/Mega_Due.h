#pragma once
#include <Arduino.h>

constexpr uint32_t SERIAL_RATE = 115200;
constexpr uint8_t RESET_LEDN_PIN = 19;  // low for on.
constexpr uint8_t RESET_LEDP_PIN = 16;  // high supply
constexpr uint8_t EEPROM_I2C_ADDR = 0x50;

#if defined(__SAM3X8E__)
constexpr uint8_t  BRIGHNESS_PWM = 5; // pins 5 & 6 are not best for PWM control.
constexpr uint8_t  CONTRAST_PWM = 6;
constexpr uint8_t PHOTO_ANALOGUE = A0;
constexpr uint8_t BRIGHTNESS = 220;
constexpr uint8_t KEYPAD_INT_PIN = 18;
constexpr uint8_t KEYPAD_ANALOGUE_PIN = A1;
constexpr uint8_t KEYPAD_REF_PIN = A3;
constexpr uint8_t RESET_5vREF_PIN = A4;
constexpr float megaFactor = 1.;

#else
#define NO_RTC
constexpr uint8_t BRIGHNESS_PWM = 6;
constexpr uint8_t CONTRAST_PWM = 7;
constexpr uint8_t PHOTO_ANALOGUE = A1;
constexpr uint8_t BRIGHTNESS = 150;
constexpr uint8_t KEYPAD_INT_PIN = 5;
constexpr uint8_t KEYPAD_ANALOGUE_PIN = A0;
constexpr uint8_t KEYPAD_REF_PIN = A2;
constexpr uint8_t RESET_5vREF_PIN = A2;
constexpr float megaFactor = 3.3f / 5.f;
#endif

inline uint8_t contrast() {  // Contrast analogRead values go from 0 to 1023, analogWrite values from 0 to 255
	//auto supplyVcorrection = 50 *  2.5 * 1024. / 3.3 / analogRead(RESET_5vREF_PIN);
	auto supplyVcorrection = (0.667 * analogRead(RESET_5vREF_PIN)) - 624;
	if (supplyVcorrection < 5) supplyVcorrection = 5;
	if (supplyVcorrection > 60) supplyVcorrection = 60;
	return uint8_t(megaFactor * supplyVcorrection);
}