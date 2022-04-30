#pragma once
#include <Arduino.h>
#include "TestDevices.h"
#include <I2C_Reset.h>
#include <Relay_Bitwise.h>
#include <Flag_Enum.h>

class HeatingSystem;

namespace Assembly {
	class Initialiser;

	class IniFunctor : public I2C_Recovery::I_IniFunctor {
	public:
		IniFunctor(Initialiser & ini) : _ini(&ini) {}
		uint8_t operator()() override; // performs post-reset initialisation  
	private:
		Initialiser * _ini;
	};

	class Initialiser {
	public:
		enum IniState {I2C_RESET, TS, POST_RESET_WARMUP, RELAYS, MIX_V, REMOTE_CONSOLES, NO_OF_INI_FLAGS};
		Initialiser(HeatingSystem & hs);
		bool state_machine_OK();
		void requiresINI(IniState ini) { _iniState.clear(ini); }
		void isOK(IniState ini) { _iniState.set(ini); }
		void resetDone(bool ok) { _iniState.set(I2C_RESET, ok); }
		uint8_t i2C_Test();
		uint8_t notify_reset(); // return 0 for OK
		HeatingSystem & hs() { return _hs; }
		I_I2Cdevice_Recovery & getDevice(uint8_t deviceAddr);
		void initialize_Thick_Consoles();
		uint8_t post_initialize_Thick_Consoles();
		uint8_t post_initialize_MixV();
		I2C_Recovery::ResetI2C _resetI2C;
		HardwareInterfaces::RelaysPort & relayPort() { return static_cast<HardwareInterfaces::RelaysPort &>(HardwareInterfaces::relayController()); }
	private:
		bool ini_DB();
		uint8_t ini_TS();
		uint8_t ini_relays();
		HeatingSystem & _hs;
		IniFunctor _iniFunctor;
		HardwareInterfaces::TestDevices _testDevices;
		flag_enum::FE_Obj<IniState, NO_OF_INI_FLAGS> _iniState;
	};

}