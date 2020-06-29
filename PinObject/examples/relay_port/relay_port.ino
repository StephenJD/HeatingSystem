#include <PinObject.h>
#include <Relay_Bitwise.h>
#include <I2C_Talk.h>
#include <I2C_Recover.h>
//#include <I2C_Scan.h>
#include <Logging.h>

#include <SD.h>

using namespace I2C_Recovery;
using namespace HardwareInterfaces;

const uint32_t SERIAL_RATE = 115200;
const uint8_t RESET_OUT_PIN = 14;  // active LOW.
const uint8_t RELAY_PORT_ADDRESS = 0x20;
const uint8_t _OK = 0;
const char * LOG_FILE = "Due.txt"; // max: 8.3

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	//static SD_Logger _log(LOG_FILE, SERIAL_RATE);
	return _log;
}

I2C_Talk i2C;
auto i2c_recover = I2C_Recover{i2C};
//I_I2C_Scan scanner{ i2C };

auto resetPin = Pin_Wag{ RESET_OUT_PIN, LOW };
Relay_B relays[] = { {1,0},{0,0},{2,0},{3,0},{4,0},{5,0},{6,0} };

namespace HardwareInterfaces {
	Bitwise_RelayController & relayController() {
		static RelaysPort _relaysPort(0x7F, i2c_recover, RELAY_PORT_ADDRESS);
		return _relaysPort;
	}
	RelaysPort & relay_port = static_cast<RelaysPort &>(relayController());
}

void setup() {
	Serial.begin(SERIAL_RATE); // required for test-all.
	resetPin.begin();

	//logger() /*<< L_allwaysFlush*/ << F(" \n\n********** Logging Begun ***********") << L_endl;
	i2C.restart();
	i2C.setTimouts(10000, 2000);
	//scanner.show_all();
	testRelays();
}

void loop() {
	testRelays();
}

void testRelays() {
	if (relay_port.initialiseDevice() == _OK) {
		const int noOfRelays = sizeof(relays) / sizeof(relays[0]);
		for (uint8_t relayNo = 0; relayNo < noOfRelays; ++relayNo) {
			//logger() << F("* Relay *") << relayNo << L_endl;
			relays[relayNo].set(1);
			if (relay_port.updateRelays() == _OK);// logger() << F(" ON OK\n"); else logger() << F(" ON Failed\n");
			delay(300);
			relays[relayNo].set(0);
			if (relay_port.updateRelays() == _OK);// logger() << F(" OFF OK\n"); else logger() << F(" OFF Failed\n");
			delay(300);
		}
	} //else logger() << F("Relay ini failed") << L_endl;
}