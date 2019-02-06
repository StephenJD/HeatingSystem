#pragma once
#include <Arduino.h>
#include "TestDevices.h"
#include "..\HardwareInterfaces\I2C_Comms.h"

class HeatingSystem;

namespace Assembly {
	class Initialiser;

	class IniFunctor : public HardwareInterfaces::I_IniFunctor {
	public:
		IniFunctor(Initialiser & ini) : _ini(&ini) {}
		uint8_t operator()() override;
	private:
		Initialiser * _ini;
	};

	class Initialiser {
	public:
		Initialiser(HeatingSystem & hs);
		uint8_t i2C_Test();
		uint8_t postI2CResetInitialisation();
		HeatingSystem & hs() { return _hs; }
		I2C_Helper::I_I2Cdevice & getDevice(uint8_t deviceAddr);

		::HardwareInterfaces::ResetI2C _resetI2C;
	private:
		uint8_t initialiseTempSensors();
		uint8_t initialiseRemoteDisplays();

		HeatingSystem & _hs;
		IniFunctor _iniFunctor;
		::HardwareInterfaces::TestDevices _testDevices;
	};

}