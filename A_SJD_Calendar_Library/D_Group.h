#if !defined (A_GROUP_H_)
#define A_GROUP_H_

///////////////////////////// Group //////////////////////////
// Single Responsibility is to manage groups of D_Data objects //
// Including subscription and insertion/deletion			   //	
/////////////////////////////////////////////////////////////////

#include "debgSwitch.h"
#include "A__Constants.h"
#include "D_EpData.h"

#if defined (ZPSIM)
	#include <iostream>
#endif

class Operations;

class Group // groups of relays, zones, Daily Programs eyc.
{
public:
	~Group();
	const U1_byte count() const;
	const U1_byte last() const;
	const U2_epAdrr startAddr() const;
	const U1_byte dataSize() const;
	void moveData(U1_ind startPos, U1_ind destination, U1_count noToMove) const;
	Operations & operator [] (int); // each member is an Operations object
	//virtual const Operations & operator [] (int) const;
	//U1_byte insertBefore(U1_byte index);
	//U1_byte pushBack();
	//U1_byte remove(U1_byte index);
	void setStartAddr(U2_epAdrr newAddr);

protected:
	Group(U1_byte noOf, Operations * first, U1_byte size);
	U1_byte noOfMembers;
	Operations * firstMember;
	U1_byte objsize;
};

////////////////////////////// GroupT Template /////////////////////////////////////

template <class T_MyOperations>
class GroupT : public Group {
public:
	GroupT(U1_byte noOf, U1_count eepromOK) // eepromOK == -1 to prevent loading defaults
		: Group(noOf, new T_MyOperations[noOf], sizeof(T_MyOperations)) {
		//debugEEprom(NO_OF_ST,NO_COUNT);	
		T_MyOperations * membAddr = static_cast<T_MyOperations *>(firstMember);
		for (U1_count i(0); i<noOf; i++) {
			membAddr[i].epD().setIndex(i);
		}
		for (U1_count i(eepromOK); i<noOf; i++) {			
			membAddr[i].loadDefaults();
		}
	};
};

// simplified template for groups that have only one member
template <class T_MyOperations> 
class GroupOfOne: public Group {
public:
	GroupOfOne(U1_count eepromOK)
		: Group(1, new T_MyOperations(), sizeof(T_MyOperations)) {
		if (eepromOK==0) {
			firstMember->loadDefaults();
		}
	};
};

#endif