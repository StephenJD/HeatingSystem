#include "I2C_Comms.h"
#include "A__Constants.h"

namespace HardwareInterfaces {

	void wait_DevicesToFinish(i2c_registers::RegAccess reg) {
		if (reg.get(R_PROG_STATE)) {
			auto timeout = Timer_mS(100);
			while (!timeout && reg.get(R_PROG_STATE)) {}
			//auto delayedBy = timeout.timeUsed();
			//logger() << L_time << "WaitedforI2C: " << delayedBy << L_endl;
			reg.set(R_PROG_STATE, 0);
		}
	};

}