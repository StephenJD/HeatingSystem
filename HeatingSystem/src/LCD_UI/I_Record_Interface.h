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
	// ***********  No CPP file for this ***********************
	// The base collection has void * to a collection of run-time data objects 
	// Persistant data is obtained from the RDB via the Query.

	/// <summary>
	/// Obtains a complete record from the DB, based on a query.
	/// Constructed with a Query, a pointer to any run-time data, and a parent object pointer.
	/// The object may be used for any of the fields returned by the query, via getField(fieldID)
	/// Derived classes provide interface to all streamable fields on a DB Query
	/// </summary>
	class I_Record_Interface
	{
	public:
		// Queries
		int parentIndex() const;
		int recordID() const { return _recordID; }
		const Query & query() const { return _recSel.query(); }
		int count() const { return _count; }
		I_Data_Formatter * getFieldAt(int fieldID, int elementIndex) const { return const_cast<I_Record_Interface *>(this)->getFieldAt(fieldID, elementIndex); }
		I_Data_Formatter * getField(int fieldID) const { return const_cast<I_Record_Interface *>(this)->getField(fieldID); }
		virtual int recordField(int selectFieldID) const { return recordID(); }
		virtual const Answer_Locator & record() const = 0;
		I_Data_Formatter * initialiseRecord(int fieldID);
		bool indexIsInDataRange(int index) { return index >= query().begin().id() && index <= query().last().id(); }

		// Polymorphic Modifiers
		virtual Answer_Locator & record() = 0;
		virtual I_Data_Formatter * getFieldAt(int fieldID, int elementIndex); // moves to first valid record at id or past id from current position
		virtual I_Data_Formatter * getField(int fieldID) = 0;
		virtual bool setNewValue(int fieldID, const I_Data_Formatter * val) = 0;
		virtual void insertNewData() {};

		TB_Status move_by(int move);
		int move_to(int id);
		int last();
		void deleteData();
		virtual int resetCount();
		void setRecordID(int id) { _recordID = id; }
		void setFieldValue(int fieldID, int value);
		Query & query() { return _recSel.query(); }
		VolatileData * runTimeData() { return _runtimeData; }
		I_Record_Interface * parent() { return _parent; }
		void setEditMode(bool inEdit) { _inEdit = inEdit; }
		bool inParentEditMode() { return _inEdit; }
	protected:
		I_Record_Interface(Query & query, VolatileData * runtimeData, I_Record_Interface * parent);
		// Data
		RecordSelector _recSel;
		VolatileData * _runtimeData; // not all classes require runtime objects
	private :
		I_Record_Interface * _parent = 0;
		int _count = 1;
		int _recordID = -1;
		bool _inEdit = false;
	};

	template<typename Record>
	class Record_Interface : public I_Record_Interface {
	public:
		typedef Record RecordType;
		Record_Interface(Query & query, VolatileData * runtimeData = 0, I_Record_Interface * parent = 0) : I_Record_Interface(query,runtimeData,parent) {}
		const Answer_R<Record> & record() const override { return _record; }
		Answer_R<Record> & record() override { return _record; }
	private:
		Answer_R<Record> _record;
	};
}
