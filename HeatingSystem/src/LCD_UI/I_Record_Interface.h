#pragma once
#include "I_Data_Formatter.h" // likely to be needed by all inheriting classes
#include "..\..\..\RDB\src\RDB.h"

namespace RelationalDatabase {
	class Answer_Locator;
	template <typename Record>
	class Answer_R;
	class Query;
}

namespace LCD_UI {
	using namespace RelationalDatabase;
	
	class VolatileData {}; // base class for all volatile data classes with UI interfaces
	constexpr VolatileData * noVolData = 0;
	extern VolatileData noVolDataObj;
	// ***********  No CPP file for this ***********************
	// The base collection has void * to a collection of run-time data objects 
	// Persistant data is obtained from the RDB via the Query.

	/// <summary>
	/// Stateless class obtaining a complete record from the DB.
	/// Derived classes provide interface to all streamable fields on a Record
	/// </summary>
	class I_Record_Interface {
	public:
		// Queries
		//I_Data_Formatter* getFieldAt(int fieldID, int elementIndex) const { return const_cast<I_Record_Interface*>(this)->getFieldAt(fieldID, elementIndex); }
		I_Data_Formatter* getField(int fieldID) const { return const_cast<I_Record_Interface*>(this)->getField(fieldID); }
		virtual int recordFieldVal(int selectFieldID) const { return answer().id(); }
		virtual const Answer_Locator& answer() const = 0;
		int recordID() const { return answer().id(); }
		TB_Status status() const { return answer().status(); }

		// Polymorphic Modifiers
		virtual Answer_Locator& answer() = 0;
		virtual I_Data_Formatter* getField(int fieldID) { return 0; }
		virtual bool setNewValue(int fieldID, const I_Data_Formatter* val) { return false; }
		virtual bool actionOn_UD(int _fieldID, int moveBy) { return false; }
		virtual bool actionOn_LR(int _fieldID, int moveBy) { return false; }
		virtual bool back(int fieldID) { return true; }
		virtual RecordSelector duplicateRecord(RecordSelector& recSel) {return {};}

		virtual VolatileData & runTimeData() { return noVolDataObj; }
	};

	template<typename Record>
	class Record_Interface : public I_Record_Interface {
	public:
		typedef Record RecordType;
		const Answer_R<Record>& answer() const override { return _record; }
		Answer_R<Record>& answer() override { return _record; }
		RecordSelector duplicateRecord(RecordSelector & recSel) override {
			return recSel.query().insert(answer().rec());
		}
	private:
		Answer_R<Record> _record;
	};

	/// <summary>
	/// Obtains a complete record from the DB, based on a query.
	/// Constructed with a RecordInterface, Query and optional parent object pointer.
	/// The object may be used for any of the fields returned by the query, via getField(fieldID)
	/// </summary>
	class Dataset {
	public:
		Dataset(I_Record_Interface & recordInterface, Query& query, Dataset * parent = 0);
		// Queries
		int ds_recordID() const { return _ds_recordID; }
		const Query& query() const { return _recSel.query(); }
		int count() const { return _count; }
		int parentIndex() const;
		const I_Record_Interface & i_record() const { return *_recInterface; }
		I_Data_Formatter* initialiseRecord(int fieldID);
		bool indexIsInDataRange(int index) { return index >= query().begin().id() && index <= query().last().id(); }

		bool actionOn_UD(int fieldID, int moveBy) { return i_record().actionOn_UD(fieldID, moveBy); }
		bool actionOn_LR(int fieldID, int moveBy) { return i_record().actionOn_LR(fieldID, moveBy); }
		bool back(int fieldID) {return i_record().back(fieldID);}
		I_Data_Formatter* getFieldAt(int fieldID, int elementIndex); // moves to first valid record at id or past id from current position
		I_Data_Formatter* getField(int fieldID) { return i_record().getField(fieldID); }
		/*virtual*/ void insertNewData();

		// Polymorphic Modifiers
		virtual bool setNewValue(int fieldID, const I_Data_Formatter* val);
		virtual void setMatchArgs() {}

		// Non polymorphic modifiers
		TB_Status move_by(int move);
		int move_to(int id);
		int last();
		void deleteData();
		/*virtual*/ int resetCount();
		Query& query() { return _recSel.query(); }
		Dataset * parent() { return _parent; }
		I_Record_Interface & i_record() { return *_recInterface; }
		void setDS_RecordID(int id) { _ds_recordID = id; }
		void setEditMode(bool inEdit) { _inEdit = inEdit; }
		bool inParentEditMode() { return _inEdit; }
	protected:
		RecordSelector _recSel;
		Dataset * _parent = 0;
		I_Record_Interface* _recInterface = 0;
		int _count = 1;
		int _ds_recordID = -1;
		bool _inEdit = false;
	};
}
