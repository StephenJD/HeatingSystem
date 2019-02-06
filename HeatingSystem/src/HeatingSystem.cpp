#include "HeatingSystem.h"
#include "Assembly\FactoryDefaults.h"
#include "HardwareInterfaces\I2C_Comms.h"
#include "HardwareInterfaces\A__Constants.h"
#include "LCD_UI\A_Top_UI.h"
#include <EEPROM\EEPROM.h>

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
	db(RDB_START_ADDR, writer, reader, VERSION)
	, _initialiser(*this)
	, mainDisplay(&_q_displays)
	, remDispl{ { i2C ,US_REMOTE_ADDRESS },{i2C ,FL_REMOTE_ADDRESS },{i2C ,DS_REMOTE_ADDRESS } }
	, _q_displays(db.tableQuery(TB_Display))
	, _tempController(db, &_initialiser._resetI2C.hardReset.timeOfReset_mS)
	, _mainPages{db, _tempController}
	, _mainConsole(localKeypad, mainDisplay, _mainPages.pages())
	, _sequencer(db, _tempController)
	{
		I2C_Helper::I_I2Cdevice::setI2Chelper(i2C);
		HardwareInterfaces::localKeypad = &localKeypad;  // required by interrupt handler
		_mainPages.setDisplay(mainDisplay);
		localKeypad.wakeDisplay(true);
		_initialiser.i2C_Test();
		serviceProfiles();
	}

void HeatingSystem::serviceConsoles() {
	_mainConsole.processKeys(); 
}

void HeatingSystem::serviceProfiles() { _sequencer.getNextEvent(); }




