#pragma once
#include "..\..\..\I2C_Helper\I2C_Helper.h"
#include "..\HardwareInterfaces\I2C_Comms.h"

namespace Assembly {
	class Initialiser;
}

class HeatingSystem;
namespace HardwareInterfaces {

	class TestDevices : public I_TestDevices {
	public:
		TestDevices(Assembly::Initialiser & ini);
		uint8_t speedTestDevices(uint8_t deviceAddr = 255);
		I2C_Helper::I_I2Cdevice & getDevice(uint8_t deviceAddr) override;
		HeatingSystem & hs();
		uint8_t testRelays();
	private:
		Assembly::Initialiser & _ini;
		int8_t showSpeedTestFailed(I2C_Helper::I_I2Cdevice & testFn, const char * device);
	};

}
