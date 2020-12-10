#include <Clock_I2C.h>
#include <Timer_mS_uS.h>

using namespace I2C_Talk_ErrorCodes;
using namespace Date_Time;

///////////////////////////////////////////////////////////////
//                     Clock_I2C                             //
///////////////////////////////////////////////////////////////

DateTime I_Clock_I2C::_timeFromRTC(int& minUnits, int& seconds) {
	uint8_t data[7] = { 0 };
	// lambda
	auto dateInRange = [](const uint8_t* data) {
		//logger() << " Read: " << int(data[0]) << "s " << int(data[1]) << "m "
		//	<< int(data[2]) << "h " << int(data[3]) << "d "
		//	<< int(data[4]) << "d " << int(data[5]) << "mn "
		//	<< int(data[6]) << "y" << L_endl;
		if (data[0] > 89) return false; // BCD secs
		if (data[1] > 89) return false;
		if (data[2] > 35) return false;
		if (data[3] > 7) return false; // Day of week
		if (data[4] > 49) return false; // BCD Day of month
		if (data[5] > 18) return false; // BCD month
		if (data[6] > 114) return false; // BCD year
		return true;
	};

	auto timeout = Timer_mS(1000);
	auto status = _OK;
	do {
		status = readData(0, 7, data);
	} while (status != _OK && !timeout);
	//logger() << F("RTC read in ") << timeout.timeUsed() << I2C_Talk::getStatusMsg(status) << L_endl;

	DateTime date{};
	if (status == _OK && dateInRange(data)) {
		date.setMins10(data[1] >> 4);
		date.setHrs(fromBCD(data[2]));
		date.setDay(fromBCD(data[4]));
		date.setMonth(fromBCD(data[5]));
		if (date.month() == 0) date.setMonth(1);
		date.setYear(fromBCD(data[6]));
		minUnits = data[1] & 15;
		seconds = fromBCD(data[0]);
		//logger() << F("RTC Time: ") << date << L_endl;
	} else {
		logger() << F("RTC Bad date.") << I2C_Talk::getStatusMsg(status) << L_endl;
	}
	return date;
}

uint8_t I_Clock_I2C::loadTime() {
	// lambda
	auto shouldUseCompilerTime = [](DateTime rtcTime, DateTime compilerTime) -> bool {
		if (rtcTime == DateTime{}) {
			if (millis() < 10000) return true; // don't want to reset RTC if this is a temporary glitch
		} else if (rtcTime < compilerTime) return true; // got valid rtc time
		return false;
	};

	int rtcMinUnits, rtcSeconds;
	auto rtcTime = _timeFromRTC(rtcMinUnits, rtcSeconds); // 0 if failed
	int compilerMinUnits, compilerSeconds;
	auto compilerTime = _timeFromCompiler(compilerMinUnits, compilerSeconds);
	auto status = _OK;
	_lastCheck_mS = millis();
	if (shouldUseCompilerTime(rtcTime, compilerTime)) {
		_now = compilerTime;
		setMinUnits(compilerMinUnits);
		setSeconds(compilerSeconds);
		saveTime();
		logger() << L_time << F(" RTC Clock Set from Compiler") << L_endl;
	} else {
		uint8_t dst;
		status = readData(8, 1, &dst);
		_now = rtcTime;
		setMinUnits(rtcMinUnits);
		setSeconds(rtcSeconds);
		_autoDST = dst >> 1;
		_dstHasBeenSet = dst & 1;
		//logger() << L_time << F(" Clock Set from RTC") << L_endl;
	}
	return status;
}

uint8_t I_Clock_I2C::saveTime() {

	uint8_t data[9];
	data[0] = toBCD(_secs); // seconds
	data[1] = (_now.mins10() << 4) + minUnits();
	data[2] = toBCD(_now.hrs());
	data[3] = 0; // day of week
	data[4] = toBCD(_now.day());
	data[5] = toBCD(_now.month());
	data[6] = toBCD(_now.year());
	data[7] = 0; // disable SQW
	data[8] = _autoDST << 1 | _dstHasBeenSet; // in RAM
	auto status = writeData(0, 9, data);
	logger() << F(" Save RTC CurrDateTime...") << I2C_Talk::getStatusMsg(status) << L_endl;

	return status;
}