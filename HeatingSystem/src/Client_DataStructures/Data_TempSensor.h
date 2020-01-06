#pragma once
#include "..\HardwareInterfaces\Temp_Sensor.h"
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace HardwareInterfaces;

//***************************************************
//              TempSensor UI Edit
//***************************************************

/// <summary>
/// This Object derivative wraps TempSensor values.
/// It is constructed with the value and a ValRange formatting object.
/// It provides streaming and editing by delegation to a file-static TempSensor_Interface object via ui().
/// </summary>
	class TempSensor_Wrapper : public I_UI_Wrapper {
	public:
		using I_UI_Wrapper::ui;
		TempSensor_Wrapper() = default;
		TempSensor_Wrapper(ValRange valRangeArg);
		I_Field_Interface & ui() override;
		
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
	//	int gotFocus(const I_UI_Wrapper * data) override;  // returns initial edit focus
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
	class TempSensor_UIinterface : public I_Field_Interface
	{
	public:
		using I_Field_Interface::editItem;
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
		//I2C_Temp_Sensor & obj(int objID) { return HardwareInterfaces::tempSensors[objID]; }
		bool operator < (R_TempSensor rhs) const { return false; }
		bool operator == (R_TempSensor rhs) const { return true; }
	};

	inline Logger & operator << (Logger & stream, const R_TempSensor & tempSensor) {
		return stream << "TempSensor: " << tempSensor.name << " Addr: " << (int)tempSensor.address;
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
		enum streamable { e_name_temp };
		Dataset_TempSensor(Query & query, VolatileData * runtimeData, I_Record_Interface * parent);
		I_UI_Wrapper * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_UI_Wrapper * val) override {return true;}
		//void insertNewData() override;
		int recordField(int) const override {
			return record().rec().address;
		}
		HardwareInterfaces::I2C_Temp_Sensor & tempSensor(int index) { return static_cast<HardwareInterfaces::I2C_Temp_Sensor*>(runTimeData())[index]; }
	private:
		TempSensor_Wrapper _tempSensor;
	};
}
