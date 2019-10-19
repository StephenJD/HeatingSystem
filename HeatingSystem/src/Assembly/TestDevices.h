#pragma once
#include "..\..\..\I2C_Talk\I2C_Talk.h"
#include "..\HardwareInterfaces\I2C_Comms.h"

namespace Assembly {
	class Initialiser;
}

class HeatingSystem;
namespace HardwareInterfaces {

	class TestDevices : public I_TestDevices {
	public:
		TestDevices(Assembly::Initialiser & ini);
		uint8_t speedTestDevices();
		I_I2Cdevice_Recovery & getDevice(uint8_t deviceAddr) override;
		HeatingSystem & hs();
		uint8_t testRelays();
	private:
		Assembly::Initialiser & _ini;
		int8_t showSpeedTestFailed(I_I2Cdevice_Recovery & testFn, const char * device);
	};

}
