#include <Clock_EP.h>
#include <EEPROM.h>
#include <Date_Time.h>

///////////////////////////////////////////////////////////////
//                     Clock_EEPROM                          //
///////////////////////////////////////////////////////////////

using namespace I2C_Talk_ErrorCodes;
using namespace Date_Time;

Clock_EEPROM::Clock_EEPROM(unsigned int addr) :_addr(addr) { /*loadTime();*/ } // loadtime crashes!

error_codes Clock_EEPROM::writer(uint16_t& address, const void* data, int noOfBytes) {
	const unsigned char* byteData = static_cast<const unsigned char*>(data);
	auto status = _OK;
	for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
#if defined(__SAM3X8E__)
		status |= eeprom().update(address, *byteData);
#else
		eeprom().update(address, *byteData);
#endif
	}
	return status;
}

error_codes Clock_EEPROM::reader(uint16_t& address, void* result, int noOfBytes) {
	uint8_t* byteData = static_cast<uint8_t*>(result);
	for (noOfBytes += address; address < noOfBytes; ++byteData, ++address) {
		*byteData = eeprom().read(address);
	}
	return _OK;
}

bool Clock_EEPROM::ok() const {
	int _day = day();
	return  _day > 0 && _day < 32;
}

uint8_t Clock_EEPROM::saveTime() {
	auto nextAddr = _addr;
	auto status = writer(nextAddr, &_now, sizeof(_now));
	status |= writer(nextAddr, &_mins1, 1);
	status |= writer(nextAddr, &_secs, 1);
	status |= writer(nextAddr, &_autoDST, 1);
	status |= writer(nextAddr, &_dstHasBeenSet, 1);
	//logger() << F("Save EEPROM CurrDateTime...") << I2C_Talk::getStatusMsg(status) << L_endl;
	return status;
}

uint8_t Clock_EEPROM::loadTime() {
	// retain and save _now unless it is invalid or before compiler-time
	auto eepromTime = _now;
	uint8_t eepromMins1;
	uint8_t eepromSecs;
	auto lastCompilerTime = _now;
	auto nextAddr = _addr;
	auto status = reader(nextAddr, &eepromTime, sizeof(_now));
	status |= reader(nextAddr, &eepromMins1, 1);
	status |= reader(nextAddr, &eepromSecs, 1);
	status |= reader(nextAddr, &_autoDST, 1);
	status |= reader(nextAddr, &_dstHasBeenSet, 1);
	reader(nextAddr, &lastCompilerTime, sizeof(_now));
	//Serial.print("Read lastCompilerTime: "); Serial.println(CStr_20(lastCompilerTime));
	if (!eepromTime.inRange() || !lastCompilerTime.inRange()) {
		eepromTime = DateTime{};
		lastCompilerTime = DateTime{};
	}
	//Serial.print("Now: "); Serial.print(CStr_20(_now)); Serial.print(" eepromTime: "); Serial.print(CStr_20(eepromTime)); Serial.print(" lastCompilerTime: "); Serial.println(CStr_20(lastCompilerTime));

	int compilerMins1, compilerSecs;
	auto compilerTime = _timeFromCompiler(compilerMins1, compilerSecs);

	if (compilerTime != lastCompilerTime) {
		_now = compilerTime;
		setMinUnits(compilerMins1);
		setSeconds(compilerSecs);
		saveTime();
		nextAddr = _addr + sizeof(_now) + 4;
		writer(nextAddr, &compilerTime, sizeof(_now));
		Serial.println(F("EE Clock Set from Compiler"));
	} else if (_now.inRange()) {
		saveTime();
		Serial.println(F("Clock saved to EEPROM"));
	} else {
		_now = eepromTime;
		setMinUnits(eepromMins1);
		setSeconds(eepromSecs);
		Serial.println(F("Clock Set from EEPROM"));
	}
	Serial.println();
	return status;
}

void Clock_EEPROM::_update() { // called every 10 minutes
	loadTime();
}