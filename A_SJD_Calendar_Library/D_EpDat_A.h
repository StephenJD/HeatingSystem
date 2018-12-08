// D_EpDat_A.h: abstract interface for the EpData classes.
//
//////////////////////////////////////////////////////////////////////

#if !defined (EDDATA_H_)
#define EDDATA_H_
 
#include "A__Constants.h"

class EpData
{
public:
	// setup methods always called on inherited class
	void setIndex(U1_ind myIndex);// {ep_index = myIndex;}
	// polymorphic methods called via baseclass pointer
	virtual char * getName() const;
	virtual void setDataStart(U2_epAdrr);
	virtual const U1_byte getNoOfItems() const = 0;	
	virtual const U1_byte getNoOfVals() const = 0;
	virtual	const U1_byte getNameSize() const = 0;

	// next two public for debug purposes only
	const U2_epAdrr getDataStart() const; // used by getVal etc.
	virtual const U1_byte getDataSize() const = 0;
	// non-polymorphic methods
	void setName(const char * name);
	void setVal(U1_ind vIndex, U1_byte val);
	U1_byte getVal(U1_ind vIndex) const;
	U1_ind index() const;
	U2_epAdrr eepromAddr() const; // required public by DateTime
	void move(U1_ind startPos, U1_ind destination, U1_count noToMove);

protected:
	virtual const U2_epAdrr getDataStartPtr() const = 0; // used by getDataStart().
	// non-polymorphic methods
	EpData();	
	bool hasNoName() const;
	
	// ********** Object Data ****************
	U1_ind ep_index;
};

////////////////////////////// EpD Template /////////////////////////////////////
// uniqueID is required to ensure that unique classes are created, 
// even if the other parameters happen to match a previously greated class.
// This is because class statics must be unique to each templated class.
/////////////////////////////////////////////////////////////////////////////////

template<U1_byte startPtr, U1_byte noOfVals, U1_byte noOfSetup = 0, U1_byte nameLength = 0, U1_byte abbrevLength = 0>
class EpDataT :  public EpData
{
public:
	EpDataT(){}
	const U1_byte getNameSize() const {return nameLength;}
protected:
	friend class NoOf_EpD;

	// ************* Accessor methods for statics needing defining for each sub class 
	//void setDataStart(U2_epAdrr start);
	const U2_epAdrr getDataStartPtr() const {return dataStartPtr;}
	const U1_byte getNoOfVals() const {return noOfVals;}
	const U1_byte getDataSize() const {return noOfVals + nameLength + abbrevLength;}
	const U1_byte getNoOfItems() const {
		return noOfSetup;
	}

	static const U2_epAdrr dataStartPtr; // Position in EEProm of first data element for this array of objects
};

template<U1_byte startPtr, U1_byte V2, U1_byte V3, U1_byte V4, U1_byte V5>
const U2_epAdrr EpDataT<startPtr, V2, V3, V4, V5>::dataStartPtr = startPtr;

//template<U1_byte startPtr, U1_byte V2, U1_byte V3, U1_byte V4, U1_byte V5>
//	EpDataT<startPtr, V2, V3, V4, V5>::EpDataT() : dataStartPtr(startPtr) {}

#endif
