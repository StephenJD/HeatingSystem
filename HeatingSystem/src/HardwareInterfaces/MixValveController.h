#pragma once
#include "Arduino.h"
#include <I2C_Helper.h>

namespace HardwareInterfaces {

	class MixValveController : public I2C_Helper::I_I2Cdevice {
	public:
		MixValveController(int addr);
		void setResetTimePtr(unsigned long * timeOfReset_mS) { 
			_timeOfReset_mS = timeOfReset_mS;
		}
		// Queries

		// Virtual Functions
		uint8_t testDevice(I2C_Helper & i2c, int addr) override;
		//uint8_t initialiseDevice() override;

	private:
		int _error = 0;
		unsigned long * _timeOfReset_mS = 0;
	};
	//extern MixValveController * mixValveController;

}