// This is the Multi-Master Arduino Mini Controller

#include <Relays.h>
#include "PinObject.h"
#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <I2C_Registers.h>
#include <I2C_Talk_ErrorCodes.h>
#include <Logging.h>
#include <Timer_mS_uS.h>

using namespace HardwareInterfaces;
using namespace arduino_logger;
using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;

RelaysSlave::RelaysSlave(I2C_Recover& i2C_recover, EEPROMClass & ep)
	: _ep(&ep)
	, _ports{ {6,HIGH,true},{7,HIGH,true},{12,HIGH,true},{13,HIGH,true},{14,HIGH,true},{15,HIGH,true},{16,HIGH,true},{17,HIGH,true} }
	{	auto reg = registers();
	reg.set(REG_8PORT_WAG_STATE, 0xFF);
	reg.set(REG_8PORT_OPORT, 0xFF);
	reg.set(REG_8PORT_OLAT, 0xFF);
	reg.set(REG_8PORT_REQ_STATE, 0xFF);
}

void RelaysSlave::begin() {
	auto reg = registers();
	logger() << F("RelaysSlave begin") << L_endl;
	logger() << "REG_8PORT_WAG_STATE: " << reg.get(REG_8PORT_WAG_STATE);
	logger() << " REG_8PORT_OPORT: " << reg.get(REG_8PORT_OPORT);
	logger() << " REG_8PORT_OLAT: " << reg.get(REG_8PORT_OLAT);
	logger() << " REG_8PORT_REQ_STATE: " << reg.get(REG_8PORT_REQ_STATE) << L_endl;
	for (auto& port : _ports) {
		port.begin(true);
	}
}

void RelaysSlave::setPorts() {
	// LOW in register = active-state
	auto reg = registers();
	auto& wagState = *reg.ptr(REG_8PORT_WAG_STATE);
	auto& portState = *reg.ptr(REG_8PORT_OPORT);
	auto& latState = *reg.ptr(REG_8PORT_OLAT);
	auto& reqState = *reg.ptr(REG_8PORT_REQ_STATE);
	//logger() << "REG_8PORT_WAG_STATE: " << wagState;
	//logger() << " REG_8PORT_OPORT: " << portState;
	//logger() << " REG_8PORT_OLAT: " << latState;
	//logger() << " REG_8PORT_REQ_STATE: " << reqState << L_endl;

	uint8_t newRequest = 0;
	bool pendingRequest = false;
	if (portState != wagState) {
		pendingRequest = true;
		newRequest = portState;
	} else if (latState != wagState) {
		logger() << "LatChangePending: " << latState << L_endl;
		pendingRequest = true;
		newRequest = latState;
	} else if (reqState != wagState) {
		logger() << "LatChangeCanceled: " << wagState << L_endl;
		reqState = wagState; // changed back to the current state!
		_lastChangeTime = millis();
	}
	// Prevent short-lived changes. A request must remain steady for > 2 seconds
	bool thisIsANewRequest = pendingRequest && newRequest != reqState;
	bool pendingRequestHasEndured = pendingRequest && millis() - _lastChangeTime > 2000;
	if (thisIsANewRequest) {
		logger() << "newRequest: " << newRequest << " at: " << millis() << L_endl;
		reqState = newRequest;
		_lastChangeTime = millis();
	} else if (pendingRequestHasEndured) {
		logger() << "ApplyRequest: " << reqState << " at: " << millis() << L_endl;
		latState = reqState;
		portState = reqState;
		wagState = reqState;
		auto  portIndex = 0;
		for (auto& port : _ports) {
			auto portRequest = wagState & (1U << portIndex);
			if (port.activeState() == 0) portRequest = ~portRequest;
			logger() << "PortIndex: " << portIndex << " PortPin: " << port.port() << " Request: " << portRequest << L_endl;
			port.set(portRequest);
			++portIndex;
		}
	}
}

void RelaysSlave::test() {
	// LOW in register = active-state
	static uint8_t setRelay = 0;
	auto reg = registers();
	auto& latState = *reg.ptr(REG_8PORT_OLAT);
	auto& wagState = *reg.ptr(REG_8PORT_WAG_STATE);
	if (wagState == latState) {
		++setRelay;
		setRelay %= 8;
		latState = ~(1U << setRelay);
		logger() << "Set: " << setRelay << " Lat: " << latState << L_endl;
	}
}
