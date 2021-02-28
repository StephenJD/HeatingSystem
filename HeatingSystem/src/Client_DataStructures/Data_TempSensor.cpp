#include "Data_TempSensor.h"
#include "Conversions.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace GP_LIB;
	using namespace HardwareInterfaces;
	using namespace I2C_Talk_ErrorCodes;

	//***************************************************
	//              Dataset_TempSensor
	//***************************************************

	RecInt_TempSensor::RecInt_TempSensor(UI_TempSensor* runtimeData)
		: _runTimeData(runtimeData)
		, _name("",5)
		, _address(0,ValRange(e_fixedWidth | e_editAll, 1, 127))
		, _temperature(0,ValRange())
		, _tempStr("", ValRange())
	{
	}

	I_Data_Formatter * RecInt_TempSensor::getField(int fieldID) {
		//if (recordID() == -1 || status() != TB_OK) return 0;
		bool canDo = status() == TB_OK;

		switch (fieldID) {
		case e_temp:
			if (canDo) {
				HardwareInterfaces::UI_TempSensor & ts = runTimeData();
				if (ts.readTemperature() != _OK ) _temperature.val = -127;
				else _temperature.val = ts.get_temp();
			}
			return &_temperature;
		case e_temp_str:
			if (canDo) {
				HardwareInterfaces::UI_TempSensor & ts = runTimeData();
				strcpy(_tempStr.str(), "`");
				auto tsErr = ts.readTemperature();
				if (tsErr == _disabledDevice) {
					ts.set_runSpeed(100000);
					tsErr = ts.readTemperature();
				}
				if (tsErr != _OK) {
					strcat(_tempStr.str(), "Err  ");
				}
				else {
					strcat(_tempStr.str(), decToString(int(ts.get_fractional_temp()/2.56),2,2).str());
				}
			}
			return &_tempStr;
		case e_name:
			if (canDo) {
				strcpy(_name.str(), answer().rec().name);
			}
			return &_name;
		default: return 0;
		}
	}
}