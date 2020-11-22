#include "Data_TempSensor.h"
#include "Conversions.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;

	//***************************************************
	//              Dataset_TempSensor
	//***************************************************

	RecInt_TempSensor::RecInt_TempSensor(VolatileData * runtimeData)
		: _runTimeData(runtimeData)
		, _name("",5)
		, _address(0,ValRange(e_fixedWidth | e_editAll, 1, 127))
		, _temperature(0,ValRange())
		, _tempStr("",4)
	{
	}

	I_Data_Formatter * RecInt_TempSensor::getField(int fieldID) {
		if (recordID() == -1 || record().status() != TB_OK) return 0;
		switch (fieldID) {
		case e_temp:
		{
			HardwareInterfaces::UI_TempSensor & ts = tempSensor(record().id());
			if (ts.readTemperature() != I2C_Talk_ErrorCodes::_OK ) _temperature.val = -127;
			else _temperature.val = ts.get_temp();
			return &_temperature;
		}
		case e_temp_str:
		{
			HardwareInterfaces::UI_TempSensor & ts = tempSensor(record().id());
			if (ts.readTemperature() != I2C_Talk_ErrorCodes::_OK) {
				strcpy(_tempStr.str(), "Err");
			}
			else {
				strcpy(_tempStr.str(), intToString(ts.get_temp()).str());
			}
			return &_tempStr;
		}

		case e_name:
		{
			strcpy(_name.str(), record().rec().name);
			return &_name;
		}
		default: return 0;
		}
	}
}