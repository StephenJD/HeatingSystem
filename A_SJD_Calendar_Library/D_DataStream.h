#pragma once

#include "A__Constants.h"
#include "D_EpManager.h" // required by classes using this template
#include "D_Operations.h"
#include "D_Edit.h"
#include "A_Stream_Utilities.h"

class EpData;
struct D_Data;

////////////////////////////// Format /////////////////////////////////////

class Format
{
public:
	static char * streamBuffer() {return fieldCText;}
protected:
	static char * formatFieldStr(char newLine = 0, S1_ind cursorOffset = 0);
	static char * createFieldStr(char * fldStr, char newLine = 0, S1_ind cursorOffset = 0);
	static U1_byte prepVal(U1_byte currentVal, bool inEdit);
	static U2_byte prepVal(U2_byte currentVal, bool inEdit);
	
	// ********** Root Class Hierarchy-Wide Static Data ****************
	static char fieldCText[DISPLAY_BUFFER_LENGTH];   // first char is reserved for cursor Pos., 2nd for NewLine
	static char * fieldText; // Write to fieldCText using fieldText as the address
	static char postText[MAX_LINE_LENGTH +4]; 
	static U1_ind cursorOffset; // position of cursor relative to begining of field.
	//static short editVal1;
};

////////////////////////////// DataStream /////////////////////////////////////

class DataStream : public Format
{
public:
	DataStream(Run & runRef);	
	virtual EpData & epD() const;
	//virtual EpManager & epM() const;
	//virtual	Run & run() const;
	virtual char * itemFldStr(S1_ind list_index, bool inEdit) const;
	virtual char * objectFldStr(S1_ind activeInd, bool inEdit) const {return (char*)"";}
	virtual S1_newPage onSelect(D_Data & d_data) {return startEdit(d_data);} // returns DO_EDIT for edit, +ve for change page, 0 do nothing
	virtual S1_newPage startEdit(D_Data & d_data) {return 0;} // returns DO_EDIT for edit, +ve for change page, 0 do nothing
	virtual S1_newPage onItemSelect(S1_byte myListPos, D_Data & d_data); // returns DO_EDIT for edit, +ve for change page, 0 do nothing
	virtual S1_newPage onNameSelect(); // returns DO_EDIT for edit, +ve for change page, 0 do nothing
	virtual bool editOnUpDown() const {return true;}
	virtual void save(){} 
	virtual void saveItem(S1_ind item);
	virtual void saveName();
	virtual void cancel() const;

	// delegated methods
	U1_ind index() const;
	void setName(const char * name);
	char * getName() const;	
	void setVal(U1_ind vIndex, U1_byte val);
	U1_byte getVal(U1_ind vIndex) const;
	const U1_byte getNoOfVals() const;
	virtual const U1_byte getNoOfStreamItems() const;

	// non-polymorphic methods
	char * nameFldStr(bool inEdit, char newLine = 0) const;
protected:	
	// ************* Virtual methods needing defining for most sub classes
	virtual ValRange addValToFieldStr(S1_ind item, U1_byte myVal = 0) const;
	virtual const char * getSetupPrompts(S1_ind item) const = 0;

	// ********** Inherited Class-Wide Static Data ****************
	// ********** Must be defined for each class   ****************
	//	static const char setupPrompts[1][13];

	// non-virtual helper methods
	static FactoryObjects * f;
	friend void setFactory(FactoryObjects *);
	Run & runRef;
};

////////////////////////////// DataStreamT /////////////////////////////////////

template<class T_EpD, class T_Run, class T_EpManager, U1_byte noOfSetup = 0>
class DataStreamT : public DataStream
{
public:		
	DataStreamT(Run & run);
	T_EpD & epDi() const {return static_cast<T_EpD &>(runRef.epD());}
	T_Run & runI() const {return static_cast<T_Run &>(runRef);}
	T_EpManager & epMi() {return epManager;}
protected:
	// ************* Virtual methods
	ValRange addValToFieldStr(S1_ind item, U1_byte myVal = 0) const;

	// ************* Accessor methods for statics needing defining for each sub class 
	const char * getSetupPrompts(S1_ind item) const;
	static const char setupPrompts[noOfSetup][13];
private:
	T_EpManager epManager;
};

template<class T_1, class T_2, class T_3, U1_byte V1>
DataStreamT<T_1,T_2,T_3,V1>::DataStreamT(Run & run) : DataStream(run){}

template<class T_1, class T_2, class T_3, U1_byte V1>
const char * DataStreamT<T_1,T_2,T_3,V1>::getSetupPrompts(S1_ind item) const {return setupPrompts[item];}