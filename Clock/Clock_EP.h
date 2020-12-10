#pragma once

#define _REQUIRES_CLOCK_
#include <Clock_.h>
#include <I2C_Talk_ErrorCodes.h>

/// <summary>
/// A Monostate class representing current time/date.
/// Last compile-time saved to EEPROM (4-bytes)
/// Saves 6-byte time to EEPROM every 10 minutes to aid recovery from re-start.
/// </summary>	
class Clock_EEPROM : public Clock {
public:
	Clock_EEPROM(unsigned int addr);
	auto saveTime()->uint8_t override;
	bool ok() const override;
private:
	void _update() override;  // called every 10 minutes - saves to EEPROM
	auto loadTime()->uint8_t override;
	auto writer(uint16_t& address, const void* data, int noOfBytes)->I2C_Talk_ErrorCodes::error_codes;
	auto reader(uint16_t& address, void* result, int noOfBytes)->I2C_Talk_ErrorCodes::error_codes;

	uint16_t _addr;
};

#ifdef EEPROM_CLOCK
inline Clock& clock_() {
	static Clock_EEPROM _clock(EEPROM_CLOCK);
	return _clock;
}
#endif