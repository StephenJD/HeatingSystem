#include "HeatingSystem.h"
#include "Assembly\FactoryDefaults.h"
#include "HardwareInterfaces\I2C_Comms.h"
#include "HardwareInterfaces\A__Constants.h"
#include "HardwareInterfaces\Logging.h"
#include "LCD_UI\A_Top_UI.h"
#include "EEPROM.h"

using namespace client_data_structures;
using namespace RelationalDatabase;

//extern Remote_display * hall_display;
//extern Remote_display * flat_display;
//extern Remote_display * us_display;

namespace HeatingSystemSupport {

	int writer(int address, const void * data, int noOfBytes) {
		const unsigned char * byteData = static_cast<const unsigned char *>(data);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
			EEPROM.update(address, *byteData);
		}
		return address;
	}

	int reader(int address, void * result, int noOfBytes) {
		uint8_t * byteData = static_cast<uint8_t *>(result);
		for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
			*byteData = EEPROM.read(address);
		}
		return address;
	}
}

using namespace	HeatingSystemSupport;
using namespace	Assembly;

HeatingSystem::HeatingSystem() 
	: 
	db(RDB_START_ADDR, 4096, writer, reader)
	,mixValveController(MIX_VALVE_I2C_ADDR)
	,remDispl{ { i2C ,US_REMOTE_ADDRESS },{i2C ,FL_REMOTE_ADDRESS },{i2C ,DS_REMOTE_ADDRESS } }
	,_initialiser(*this)
	,_mainPages{db}
	,_mainConsole(localKeypad, mainDisplay, _mainPages.pages())
	{
	HardwareInterfaces::localKeypad = &localKeypad;  // required by interrupt handler
	}

void HeatingSystem::serviceMainConsole() { _mainConsole.processKeys(); }



