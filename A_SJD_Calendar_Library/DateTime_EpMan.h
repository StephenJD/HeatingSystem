#pragma once
#include "debgSwitch.h"
#include "A__Constants.h"
#include "D_EpManager.h"

class DT_EpMan : public EpManager {
public:
	static U1_ind copyItem(U1_ind zone, U1_ind & copyFrom);

private:	
	U1_ind getInsertPos(S2_byte gap, U1_ind parent, U1_ind insertPos);
	void moveData(U1_ind startPos, U1_ind destination,U1_byte noToCopy);
	void modifyReferences(U1_ind startPos, U1_ind destination, S1_ind modify);
	void initializeNewItem(U1_ind insertPos);
	U1_ind getLocalSpareItem(U1_ind myDP);
	U1_ind findSpareItem(); // Returns position of first space. Returns 0 if no space exists
	U1_ind createNewItem();

};

#if defined (ZPSIM)
	void debugDTs(U1_ind zone, char* msg);
#endif