#include "I2C_Comms.h"
#include "A__Constants.h"

namespace HardwareInterfaces {

	void wait_DevicesToFinish(i2c_registers::RegAccess reg) {
		if (reg.get(R_PROG_WAITING_FOR_REMOTE_I2C_COMS)) {
			auto timeout = Timer_mS(100);
			while (!timeout && reg.get(R_PROG_WAITING_FOR_REMOTE_I2C_COMS)) {}
			//auto delayedBy = timeout.timeUsed();
			//logger() << L_time << "WaitedforI2C: " << delayedBy << L_endl;
			reg.set(R_PROG_WAITING_FOR_REMOTE_I2C_COMS, 0);
		}
	};

}