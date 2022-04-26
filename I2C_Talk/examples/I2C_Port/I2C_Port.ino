#include <I2C_Talk.h>
//#include <I2C_Helper.h>
#include <Logging.h>

const uint32_t SERIAL_RATE = 9600;
const uint8_t RESET_I2C_PIN = 14;  // active LOW.
const uint8_t RELAY_PORT_ADDRESS = 0x20;
constexpr uint8_t REG_8PORT_IODIR = 0x00; // default all 1's = input
constexpr uint8_t REG_8PORT_PullUp = 0x06;
constexpr uint8_t REG_8PORT_OPORT = 0x09;
constexpr uint8_t REG_8PORT_OLAT = 0x0A;
const uint8_t _OK = 0;

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	return _log;
}

I2C_Talk i2C;
//I2C_Helper i2C;

uint8_t relays[] = { 1,0,2,3,4,5,6};

void setup() {
	Serial.begin(SERIAL_RATE); // required for test-all.
	pinMode(RESET_I2C_PIN, OUTPUT);
	digitalWrite(RESET_I2C_PIN, HIGH);
	logger() << L_allwaysFlush << "Start" << L_endl;
	//I2C_Talk i2C;
	//I2C_Helper i2C;
	i2C.begin();
	uint8_t allZero = 0;
	uint8_t allOnes = 0xFF;
	auto status = i2C.write_verify(RELAY_PORT_ADDRESS, REG_8PORT_PullUp, 1, &allZero);
	logger() << "\nPullups off" << i2C.getStatusMsg(status) << L_endl; // clear all pull-up resistors
	status = i2C.write_verify(RELAY_PORT_ADDRESS,REG_8PORT_OLAT, 1, &allOnes); // set latches High
	logger() << "Olats High" << i2C.getStatusMsg(status) << L_endl; 
	status = i2C.write_verify(RELAY_PORT_ADDRESS, REG_8PORT_IODIR, 1, &allZero); // All Output
	logger() << "Olats OUT"  << i2C.getStatusMsg(status) << L_endl << L_endl; // clear all pull-up resistors
	
	while (true) {	
		testRelays(i2C);
	}
}

void loop() {
	//testRelays();
}

void testRelays(I2C_Talk & i2C) {
	const int noOfRelays = sizeof(relays) / sizeof(relays[0]);
	for (uint8_t relayNo = 0; relayNo < noOfRelays; ++relayNo) {
		uint8_t _relayRegister = 0xff & ~(1<<relays[relayNo]);
		auto status = i2C.write_verify(RELAY_PORT_ADDRESS,REG_8PORT_OLAT, 1, &_relayRegister);
		logger() << "Set " << _relayRegister << i2C.getStatusMsg(status) << L_endl; // clear all pull-up resistors
		delay(300);
		_relayRegister = 0xff;
		status = i2C.write_verify(RELAY_PORT_ADDRESS, REG_8PORT_OLAT, 1, &_relayRegister);
		logger() << "Clear " << _relayRegister << i2C.getStatusMsg(status) << L_endl; // clear all pull-up resistors
		delay(300);
	}
}