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
	, mainDisplay(&_q_displays)
	, mixValveController(MIX_VALVE_I2C_ADDR)
	, remDispl{ { i2C ,US_REMOTE_ADDRESS },{i2C ,FL_REMOTE_ADDRESS },{i2C ,DS_REMOTE_ADDRESS } }
	, _q_displays(db.tableQuery(TB_Display))
	, _initialiser(*this)
	, _mainPages{db}
	, _mainConsole(localKeypad, mainDisplay, _mainPages.pages())
	{
		HardwareInterfaces::localKeypad = &localKeypad;  // required by interrupt handler
		_mainPages.setDisplay(mainDisplay);
		localKeypad.wakeDisplay(true);
		//int zoneIndex = Z_UpStairs;
		//for (Answer_R<R_Zone> zoneRec : db.tableQuery(TB_Zone)) {
		//	zoneArr[zoneIndex].initialise(zoneRec.id(), 0, 0);
		//}
	}

void HeatingSystem::serviceConsoles() {
	_mainConsole.processKeys(); 
}

void HeatingSystem::serviceProfiles() {}

void HeatingSystem::serviceTemperatureController() {
	// get curr temps
	//for (auto zone : zoneArr) {
	//	auto zoneID = zone.record();
	//	R_Zone thisZone;
	//	db.table(TB_Zone).readRecord(zoneID, &thisZone);
	//	auto callTempSensorID = thisZone.callTempSens;
	//	R_TempSensor thisTS;
	//	db.table(TB_TempSensor).readRecord(callTempSensorID, &thisTS);
	//	//auto TSAddr = thisTS.obj(0);
	//	zone.setCurrTemp(tempSensorArr[0].get_temp());
	//}
	// check mixer temps
}



