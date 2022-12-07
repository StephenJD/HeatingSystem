#include "I2C_Comms.h"
#include "A__Constants.h"

namespace arduino_logger {
	Logger& loopLogger();
}
using namespace arduino_logger;

namespace HardwareInterfaces {

	void wait_DevicesToFinish(i2c_registers::RegAccess reg) {
		if (reg.get(R_PROG_WAITING_FOR_REMOTE_I2C_COMS)) {
			//loopLogger() << "\ttimeout100" << L_endl;
			auto timeout = Timer_mS(100);
			while (!timeout && reg.get(R_PROG_WAITING_FOR_REMOTE_I2C_COMS)) {
				//loopLogger() << "\t" << timeout.timeUsed() << " mS used. Millis: " << millis() << L_endl;
			}
			//auto delayedBy = timeout.timeUsed();
			//logger() << L_time << "WaitedforI2C: " << delayedBy << L_endl;
			reg.set(R_PROG_WAITING_FOR_REMOTE_I2C_COMS, 0);
			//loopLogger() << "\tProgWait Set:0: " << L_endl;
		} //else loopLogger() << "\tProgWait:0" << L_endl;
	};

}