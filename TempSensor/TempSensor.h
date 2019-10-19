#pragma once
// #include "I2C_Talk.h" // only for #def ZPSIM
#include <Arduino.h>

class I2C_Talk;

class iTemp_Sensor {
public:
	int8_t get_temp() const;
	virtual int16_t get_fractional_temp() const = 0;
	int lastError() {return error;}
protected:
	iTemp_Sensor();
	mutable int16_t lastGood;
	static int error;
	#ifdef ZPSIM
	mutable int16_t change;
	#endif
};

class I2C_Temp_Sensor : public iTemp_Sensor {
public:
	explicit I2C_Temp_Sensor(int address, bool is_hi_res = false);
	static void setI2C(I2C_Talk & i2C);
	virtual int16_t get_fractional_temp() const;
	uint8_t getAddress() { return address; }
	void setAddress(uint8_t newAddr) { address = newAddr; }
private:
	static I2C_Talk * i2C;
	uint8_t address;
};

