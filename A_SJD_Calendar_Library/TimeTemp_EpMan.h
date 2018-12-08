#pragma once

#include "A__Constants.h"
#include "D_EpManager.h"

class TT_EpMan : public EpManager {
public:
	static U1_ind copyItem(U1_ind dp, U1_byte copyPos, S1_byte move); // copyPos is 0-n relative to DP's firstTT
	static void recycle(U1_ind index);
	static void sortTTs(U1_ind dp);
private:
	//static void	moveTTs(U1_ind start, U1_ind destination, U1_count noToMove);
	static void swapTTs(U1_ind i, U1_ind j);
	U1_ind getInsertPos(S2_byte gap, U1_ind parent, U1_ind insertPos);
	void moveData(U1_ind startPos, U1_ind destination,U1_byte noToCopy);
	void modifyReferences(U1_ind startPos, U1_ind destination, S1_ind modify);
	void initializeNewItem(U1_ind insertPos);
	U1_ind getLocalSpareItem(U1_ind myDP);
	U1_ind findSpareItem(); // Returns position of first space. Returns 0 if no space exists
	U1_ind createNewItem();
};

#if defined (DEBUG_INFO)
void debugTTs(char* msg);
#endif