#include "Relay_Bitwise.h"
#include "I2C_RecoverRetest.h"
#include <PinObject.h>

#include <Logging.h>

constexpr uint8_t REG_8PORT_IODIR = 0x00; // default all 1's = input
constexpr uint8_t REG_8PORT_PullUp = 0x06;
constexpr uint8_t REG_8PORT_OPORT = 0x09;
constexpr uint8_t REG_8PORT_OLAT = 0x0A;
using namespace arduino_logger;

namespace HardwareInterfaces {
	using namespace I2C_Recovery;
	using namespace I2C_Talk_ErrorCodes;

	/////////////////////////////////////
	//       Relay_B Functions      //
	/////////////////////////////////////

	Relay_B::Relay_B(int port, bool activeState) : Flag(port, activeState) { set(false); }
	Relay_B::Relay_B(uint8_t relayRec) : Flag(relayRec) { set(false); }

	void Relay_B::initialise(int port, bool activeState) {
		Flag::initialise(port, activeState);
		set(false);
	}

	void Relay_B::initialise(uint8_t relayRec) {
		Flag::initialise(relayRec);
		set(false);
	}

	bool Relay_B::set(bool state) { // returns true if state is changed
		if (Flag::set(state)) {
			relayController().registerRelayChange(*this);
			return true;
		}
		else return false;
	}

	void Relay_B::checkControllerStateCorrect() {
		relayController().registerRelayChange(*this);
	}

	void Relay_B::getStateFromContoller() {
		set(relayController().portState(*this));
	}

	
	/////////////////////////////////////
	//       Bitwise_RelayController Functions      //
	/////////////////////////////////////
	
	void Bitwise_RelayController::registerRelayChange(Flag relay) {
		RelayPortWidth_T relayFlagMask = 1 << relay.port();
		if (relay.controlState()) {  // set this bit
			_relayRegister |= relayFlagMask;
		}
		else {// clear bit
			_relayRegister &= ~relayFlagMask;
		}
		//logger() << L_time << F("set RelayPort: ") << port() << F(" to ") << state << F(" bits: 0x") << L_hex << RelaysPort::relayRegister << L_endl;
	}

	bool Bitwise_RelayController::portState(Flag relay) {
		RelayPortWidth_T relayFlagMask = 1 << relay.port();
		bool physicalState = _relayRegister & relayFlagMask;
		return !(relay.activeState() ^ physicalState);
	}

	/////////////////////////////////////
	//       RelaysPort Functions      //
	/////////////////////////////////////

	RelaysPort::RelaysPort(RelayPortWidth_T connected_relays, I2C_Recover & recovery, int addr)
		: Bitwise_RelayController(connected_relays)
		, I_I2Cdevice_Recovery{ recovery, addr }
	{}

	auto RelaysPort::testDevice()->I2C_Talk_ErrorCodes::Error_codes {// non-recovery test
		return I_I2Cdevice::write_verify(REG_8PORT_OLAT, 1, &_relayRegister); // set latches
	}

	Error_codes RelaysPort::initialiseDevice() {
		return _OK;
	}

	Error_codes RelaysPort::updateRelays() {
		//writeInSync();
		//logger() <<  L_time << F("updateRelays") << L_endl;
		auto status = reEnable(true);
		if (status == _OK) {
			//logger() << "write_verify OLAT..." << L_endl;
			status = write_verify(REG_8PORT_OLAT, 1, &_relayRegister);
			//logger() << "write_verify OLAT done." << L_endl;
		} else {
			logger() << L_time << F("RelaysPort::updateRelays() failed Reg: 0x") << L_hex << ~_relayRegister << I2C_Talk::getStatusMsg(status) << L_endl;
		}
		return status;
	}

	auto RelaysPort::readPorts()->I2C_Talk_ErrorCodes::Error_codes {
			return read(REG_8PORT_OLAT, 1, &_relayRegister);
	}

}