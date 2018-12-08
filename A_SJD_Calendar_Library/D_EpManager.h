#pragma once

#include "A__Constants.h"
class FactoryObjects;
//enum StartAddr: unsigned char;

/*
EEProm addresses need 11 bits (4095)
TT's could be linked using the 8th bit of temp to indicate a link.
DP's could be linked using the 8th bit of NoOfTTs to indicate a link.
DT's could be linked using the 8th bit of DPIndex to indicate a link.
This would allow additional ordered lists to be added without moving everything along.
However, it would require indexing to follow links from the start.
On balance, I think it better / simpler to shift stuff along to insert new data, rather than use links.
The "hit" is taken by the insertion, not by every access to an element.
*/

class EpManager
{
public:
	EpManager();
	U1_ind insertNewItem(U1_ind myParent, U1_ind & originalPos, bool moveToInsertPos = false); //U1_ind & originalPos = 0
protected:
	static U1_ind makeSpaceBelow(StartAddr dataStart, U1_byte space);
	static EpManager * thisObject;
	static FactoryObjects * f;
	friend void setFactory(FactoryObjects *);
private:
	virtual S2_byte getNewItem(U1_ind myParent);
	virtual U1_ind getInsertPos(S2_byte gap, U1_ind parent, U1_ind insertPos);
	virtual void moveData(U1_ind startPos, U1_ind destination,U1_byte noToCopy){}
	virtual void initializeNewItem(U1_ind insertPos){}
	virtual U1_ind findSpareItem() {return 0;}
	virtual U1_ind getLocalSpareItem(U1_ind myParent) {return 0;};
	virtual void modifyReferences(U1_ind startPos, U1_ind destination, S1_ind modify){}
	virtual U1_ind createNewItem() {return 0;}
};

class IniSet_EpMan : public EpManager {
public:
};


class NoOf_EpMan : public EpManager {
public:
};

class Display_EpMan : public EpManager {
public:
};

class MixValve_EpMan : public EpManager {
public:
};

class OccasHtr_EpMan : public EpManager {
public:
};

class Relay_EpMan : public EpManager {
public:
};

class TempSens_EpMan : public EpManager {
public:
};

class ThrmStore_EpMan : public EpManager {
public:
};

class TowelR_EpMan : public EpManager {
public:
};

class Zone_EpMan : public EpManager {
public:
	static U1_ind copyItem(U1_ind zone, U1_byte copyFrom = 0);
	static void swapDPs(U1_ind i, U1_ind j);
};