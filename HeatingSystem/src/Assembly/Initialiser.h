#pragma once
#include <Arduino.h>
#include "TestDevices.h"
#include "..\HardwareInterfaces\I2C_Comms.h"
#include <Relay_Bitwise.h>

class HeatingSystem;

namespace Assembly {
	class Initialiser;

	class IniFunctor : public HardwareInterfaces::I_IniFunctor {
	public:
		IniFunctor(Initialiser & ini) : _ini(&ini) {}
		uint8_t operator()() override; // performs post-reset initialisation  
	private:
		Initialiser * _ini;
	};

	class Initialiser {
	public:
		Initialiser(HeatingSystem & hs);
		uint8_t i2C_Test();
		uint8_t postI2CResetInitialisation(); // return 0 for OK
		HeatingSystem & hs() { return _hs; }
		I_I2Cdevice_Recovery & getDevice(uint8_t deviceAddr);
		void initialize_Thick_Consoles();
		HardwareInterfaces::ResetI2C _resetI2C;
		HardwareInterfaces::RelaysPort & relayPort() { return static_cast<HardwareInterfaces::RelaysPort &>(HardwareInterfaces::relayController()); }
	private:
		uint8_t initialize_Thin_Consoles(); // return 0 for OK
		uint8_t initialise_slaveConsole_TempSensors();
		//uint8_t initialiseMixValveController(); // return 0 for OK

		HeatingSystem & _hs;
		IniFunctor _iniFunctor;
		HardwareInterfaces::TestDevices _testDevices;
	};

}