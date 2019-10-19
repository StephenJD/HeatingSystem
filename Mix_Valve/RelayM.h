#pragma once

class iRelay {
public:
	enum { e_On = 1, e_Off = 0 };
	virtual void set(bool on) = 0;
	virtual bool get() = 0;
};

class Relay : public iRelay {
public:
	Relay(int address);
	virtual void set(bool on);
	virtual bool get();
private:
	unsigned char address;
};

