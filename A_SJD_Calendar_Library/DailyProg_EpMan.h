#pragma once

#include "A__Constants.h"
#include "D_EpManager.h"

class DP_EpMan : public EpManager {
public:
	static U1_ind copyItem(U1_ind zone, U1_byte & copyIndex, bool noAdoption = false);
	static void recycleDP(U1_ind zIndex, U1_ind dpIndex);
	static S1_inc removeDuplicateDays(U1_ind zIndex, U1_ind dpIndex);
	static U1_ind startOfGroup(U1_ind dtInd); // returns index of first weekly DP in current group.
	static U1_ind lastInGroup(U1_ind dpIndex); // returns index of last weekly DP in current group.
	//static bool onlyOneDay(U1_ind dp);
private:	
	static U1_ind findZoneForDP(U1_ind dpIndex);
	static U1_byte missingDays(U1_ind dpGroupStartIndex);
	//static void	moveDPs(U1_ind start, U1_ind destination, U1_count noToMove);
	static void swapDPs(U1_ind i, U1_ind j);
	static void sortDPs(U1_ind dpGroupStartIndex);
	static void resetEndOfGroup(U1_ind dpGroupStartIndex);
	
	U1_ind getInsertPos(S2_byte gap, U1_ind parent, U1_ind insertPos);
	void moveData(U1_ind startPos, U1_ind destination,U1_byte noToCopy);
	void modifyReferences(U1_ind startPos, U1_ind destination, S1_ind modify);
	void initializeNewItem(U1_ind insertPos);
	U1_ind getLocalSpareItem(U1_ind myDP);
	U1_ind findSpareItem(); // Returns position of first space. Returns 0 if no space exists
	U1_ind createNewItem();

	static U1_ind recipientOfTT(U1_ind thisTT);

};
