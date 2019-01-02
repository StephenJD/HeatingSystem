#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\LCD_UI\ValRange.h"
#include "..\..\..\RDB\src\RDB.h"
#include "..\..\..\DateTime\src\Date_Time.h"

#ifdef ZPSIM
	#include <ostream>
#endif

namespace client_data_structures {
	using namespace LCD_UI;
	//***************************************************
	//              Zone UI Edit
	//***************************************************

	/// <summary>
	/// This Object derivative wraps ReqIsTemp values.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static ReqIsTemp_Interface object via ui().
	/// </summary>
	class ReqIsTemp_Wrapper : public I_UI_Wrapper {
	public:
		using I_UI_Wrapper::ui;
		ReqIsTemp_Wrapper() = default;
		ReqIsTemp_Wrapper(ValRange valRangeArg);
		I_Field_Interface & ui() override;

		char name[7];
		uint8_t isTemp;
		bool isHeating;
	};

	/// <summary>
	/// This Collection_Hndl derivative provides editing of ReqIsTemp fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	class Edit_ReqIsTemp_h : public Edit_Int_h {
	public:
		using I_Edit_Hndl::currValue;
		int gotFocus(const I_UI_Wrapper * data) override;  // returns initial edit focus
		int cursorFromFocus(int focusIndex) override;
		void setInRangeValue() override;
	private:
	};

	/// <summary>
	/// This LazyCollection derivative is the UI for ReqIsTemp
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the ReqIsTemp.
	/// </summary>
	class ReqIsTemp_Interface : public I_Field_Interface {
	public:
		using I_Field_Interface::editItem;
		const char * streamData(const Object_Hndl * activeElement) const override;
		I_Edit_Hndl & editItem() { return _editItem; }
	protected:
		Edit_ReqIsTemp_h _editItem;
	};

	//***************************************************
	//              Zone Dynamic Class
	//***************************************************

	class Zone : public LCD_UI::VolatileData {
	public:
		Zone(int tempReq, int currTemp);
		uint8_t getCallFlowT() const { return _callFlowTemp; } // only for simulator & reporting
		//int8_t getFlowSensTemp() const;
		uint8_t getCurrProgTempRequest() const { return _currProgTempRequest; }
		uint8_t getCurrTempRequest() const { return _currTempRequest; }
		uint8_t getCurrTemp() const { return _currTemp; }
		bool isCallingHeat() const { return _currTemp < _currTempRequest; }
		// Modifier
		void setCurrTempRequest(uint8_t val) { _currTempRequest = val; }

	private:
		int8_t _currProgTempRequest;
		int8_t _currTempRequest;
		int8_t _currTemp;
		int8_t _nextTemp;
		Date_Time::DateTime _ttEndDateTime;
		uint8_t _callFlowTemp;		// Programmed flow temp, modified by zoffset
		uint8_t _tempRatio;
		long _aveError; // in 1/16 degrees
		long _aveFlow;  // in degrees
		//startTime * _sequence;
		bool _isHeating; // just for logging
		//GetExpCurveConsts _getExpCurve;
	};

	//extern Zone * zones; // Array of Zones provided by client

	//***************************************************
	//              Zone RDB Tables
	//***************************************************

	struct R_Zone {
		char name[7];
		char abbrev[5];
		RecordID callTempSens;
		RecordID callRelay;
		RecordID mixValve;
		int8_t offsetT;
		int8_t autoFinalT;
		int8_t autoTimeC;
		int8_t autoMode;
		int8_t manHeatTime;
		//Zone & obj(int objID) { return zones[objID]; }
		bool operator < (R_Zone rhs) const { return false; }
		bool operator == (R_Zone rhs) const { return true; }
	};

#ifdef ZPSIM
	inline std::ostream & operator << (std::ostream & stream, const R_Zone & zone) {
		return stream << "Zone: " << zone.name;
	}
#endif

	struct R_DwellingZone {
		RecordID dwellingID;
		RecordID zoneID;
		RecordID field(int fieldIndex) {
			if (fieldIndex == 0) return dwellingID; else return zoneID;
		}
		bool operator < (R_DwellingZone rhs) const { return false; }
		bool operator == (R_DwellingZone rhs) const { return true; }
	};

#ifdef ZPSIM
	inline std::ostream & operator << (std::ostream & stream, const R_DwellingZone & dwellingZone) {
		return stream << "DwellingZone DwID: " << std::dec << (int)dwellingZone.dwellingID << " ZnID: " << (int)dwellingZone.zoneID;
	}
#endif

	//***************************************************
	//              Zone DB Interface
	//***************************************************

	/// <summary>
	/// DB Interface to all Zone Data
	/// Provides streamable fields which may be poulated by a database or the runtime-data.
	/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
	/// A single object may be used to stream and edit any of the fields via getFieldAt
	/// </summary>
	class Dataset_Zone : public Record_Interface<R_Zone> {
	public:
		enum streamable { e_name, e_abbrev, e_reqTemp, e_factor, e_reqIsTemp };
		Dataset_Zone(Query & query, VolatileData * runtimeData, I_Record_Interface * parent);
		I_UI_Wrapper * getField(int _fieldID) override;
		bool setNewValue(int _fieldID, const I_UI_Wrapper * val) override;
		Zone & zone(int index) { return static_cast<Zone*>(runTimeData())[index]; }
	private:
		StrWrapper _name;
		StrWrapper _abbrev;
		IntWrapper _requestTemp;
		DecWrapper _factor;
		ReqIsTemp_Wrapper _reqIsTemp;
	};

}