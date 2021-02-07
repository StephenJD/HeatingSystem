#pragma once
#include <TempSensor.h>
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"

namespace HardwareInterfaces {
	//***************************************************
	//              UI_TempSensor
	//***************************************************

	class UI_TempSensor : public TempSensor, public LCD_UI::VolatileData {
	public:
		using TempSensor::TempSensor;
		UI_TempSensor() = default;
#ifdef ZPSIM
		UI_TempSensor(I2C_Recovery::I2C_Recover & recover, uint8_t addr, int16_t temp) : TempSensor(recover, addr) { _lastGood = temp << 8; }
#endif
		// Queries
		bool operator== (const UI_TempSensor & rhs) const { return _recordID == rhs._recordID; }

		// Modifiers
		void initialise(int recordID, int address) { _recordID = recordID; TempSensor::initialise(address); }

	private:
		RelationalDatabase::RecordID _recordID = 0;
	};

	//extern UI_TempSensor * tempSensors; // Array of TempSensor provided by client
}

namespace client_data_structures {
	using namespace LCD_UI;

	//***************************************************
	//              TempSensor RDB Tables
	//***************************************************

	struct R_TempSensor {
		char name[5];
		uint8_t address; // Initialisation inhibits aggregate initialisation.
		//UI_TempSensor & obj(int objID) { return HardwareInterfaces::tempSensors[objID]; }
		bool operator < (R_TempSensor rhs) const { return false; }
		bool operator == (R_TempSensor rhs) const { return true; }
	};

	inline Logger & operator << (Logger & stream, const R_TempSensor & tempSensor) {
		return stream << F("TempSensor: ") << tempSensor.name << F(" Addr: ") << (int)tempSensor.address;
	}

	//***************************************************
	//              RecInt_TempSensor
	//***************************************************

	/// <summary>
	/// DB Interface to all TempSensor Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>
	class RecInt_TempSensor : public Record_Interface<R_TempSensor>
	{
	public:
		enum streamable { e_name, e_temp, e_temp_str, e_addr };
		RecInt_TempSensor(HardwareInterfaces::UI_TempSensor* runtimeData);
		HardwareInterfaces::UI_TempSensor& runTimeData() override { return _runTimeData[recordID()] ; }

		I_Data_Formatter * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_Data_Formatter * val) override {return true;}
		int recordFieldVal(int) const override {
			return answer().rec().address;
		}
	private:
		HardwareInterfaces::UI_TempSensor* _runTimeData;
		StrWrapper _name;
		IntWrapper _address;
		IntWrapper _temperature;
		StrWrapper _tempStr;
	};

}
