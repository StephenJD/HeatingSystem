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
//              TempSensor UI Edit
//***************************************************

/// <summary>
/// This Object derivative wraps TempSensor values.
/// It is constructed with the value and a ValRange formatting object.
/// It provides streaming and editing by delegation to a file-static TempSensor_Interface object via ui().
/// </summary>
	class TempSensor_Wrapper : public I_Data_Formatter {
	public:
		using I_Data_Formatter::ui;
		TempSensor_Wrapper() = default;
		TempSensor_Wrapper(ValRange valRangeArg);
		I_Streaming_Tool & ui() override;
		
		char name[5];
		uint8_t address;
		int8_t temperature;
	};

	/// <summary>
	/// This Collection_Hndl derivative provides editing of ReqIsTemp fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	//class Edit_TempSensor_h : public Edit_Int_h {
	//public:
	//	using I_Edit_Hndl::currValue;
	//	int gotFocus(const I_Data_Formatter * data) override;  // returns initial edit focus
	//	int cursorFromFocus(int focusIndex) override;
	//	void setInRangeValue() override;
	//private:
	//};

	//***************************************************
	//              TempSensor LCD_UI
	//***************************************************

	/// <summary>
	/// This LazyCollection derivative is the UI for ReqIsTemp
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the ReqIsTemp.
	/// </summary>
	class TempSensor_UIinterface : public I_Streaming_Tool
	{
	public:
		using I_Streaming_Tool::editItem;
		const char * streamData(bool isActiveElement) const override;
		I_Edit_Hndl & editItem() override { return _editItem; }
	protected:
		Edit_Int_h _editItem;
	};	
	
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
//              Dataset_TempSensor
//***************************************************

/// <summary>
/// DB Interface to all TempSensor Data
/// Provides streamable fields which may be populated by a database or the runtime-data.
/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
/// A single object may be used to stream and edit any of the fields via getField
/// </summary>
	class Dataset_TempSensor : public Record_Interface<R_TempSensor>
	{
	public:
		enum streamable { e_name_temp, e_temp };
		Dataset_TempSensor(Query & query, VolatileData * runtimeData, I_Record_Interface * parent);
		I_Data_Formatter * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_Data_Formatter * val) override {return true;}
		//void insertNewData() override;
		int recordField(int) const override {
			return record().rec().address;
		}
		HardwareInterfaces::UI_TempSensor & tempSensor(int index) { return static_cast<HardwareInterfaces::UI_TempSensor*>(runTimeData())[index]; }
	private:
		TempSensor_Wrapper _tempSensor;
	};

}
