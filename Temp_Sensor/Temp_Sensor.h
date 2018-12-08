#pragma once
#include "I2C_Helper.h" // only for #def ZPSIM
#include "A__Typedefs.h"
class I2C_Helper;

class iTemp_Sensor {
public:
	S1_byte get_temp() const;
	virtual S2_byte get_fractional_temp() const = 0;
	int lastError() {return error;}
protected:
	iTemp_Sensor();
	mutable S2_byte lastGood;
	static int error;
	#ifdef ZPSIM
	mutable S2_byte change;
	#endif
};

class I2C_Temp_Sensor : public iTemp_Sensor {
public:
	explicit I2C_Temp_Sensor(int address, bool is_hi_res = false);
	static void setI2C(I2C_Helper & i2C);
	virtual S2_byte get_fractional_temp() const;
	U1_byte getAddress() { return address; }
	void setAddress(U1_byte newAddr) { address = newAddr; }
private:
	static I2C_Helper * i2C;
	U1_byte address;
};

