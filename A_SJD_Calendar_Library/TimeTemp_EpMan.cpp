#include "debgSwitch.h"
#include "TimeTemp_EpMan.h"
#include "Zone_Run.h"
#include "EEPROM_Utilities.h"
#include "D_Factory.h"
#include "DailyProg_EpMan.h"
#include "DailyProg_Stream.h"

using namespace EEPROM_Utilities;

U1_ind TT_EpMan::copyItem(U1_ind dp, U1_byte copyPos, S1_byte move) {// copyPos is 0-n relative to DP's E_firstTT
	// look for unused TT

#if defined DEBUG_INFO
		std::cout << "\nCopyTT requested from TT[" << int(f->dailyPrograms[dp].getVal(E_firstTT)+copyPos) << "] for DP " << (int)dp;
		std::cout << " has NoOfTTs: " << (int)f->dailyPrograms[dp].getVal(E_noOfTTs)  << " Starting at " << (int)f->dailyPrograms[dp].getVal(E_firstTT)  <<'\n';
		std::cout << "CurrDT : " << (int)Zone_Stream::currZdtData.currDT << "\n";
		debugTTs("");
#endif
	U1_ind insertPos = 0;
	U1_ind newInd = f->timeTemps[0].epM().insertNewItem(dp,insertPos);
	#if defined (DEBUG_INFO)
		std::cout << "Given new TT at: " << (int)newInd << " NoOfTTs: " << (int)f->dailyPrograms[dp].getVal(E_noOfTTs) << '\n';
	#endif
	copyPos = copyPos + f->dailyPrograms[dp].getVal(E_firstTT);
	if (newInd != 0) { // copy TT data 
		DTtype newTime = f->timeTemps[copyPos].getVal(E_ttTime);
		if (newTime == SPARE_TT) {
			f->timeTemps[newInd].setVal(E_ttTime,56); // 7 am.
			f->timeTemps[newInd].setVal(E_ttTemp,29); // 19deg
		} else {
			newTime = newTime.addDateTime(move);
			f->timeTemps[newInd].setVal(E_ttTime,newTime.U1());
			f->timeTemps[newInd].setVal(E_ttTemp,f->timeTemps[copyPos].getVal(E_ttTemp));
		}
		sortTTs(dp);
	}
#if defined DEBUG_INFO
		debugTTs("Done copy TT");
#endif	
		return newInd;
} 

void TT_EpMan::recycle(U1_ind index){
	f->timeTemps[index].setVal(E_ttTime,SPARE_TT);
	#if defined (DEBUG_INFO)
		std::cout << "Recycle TT at: " << (int)index << '\n';
	#endif
}

void TT_EpMan::swapTTs(U1_ind i, U1_ind j) {
	swapData(f->timeTemps[i].epD().eepromAddr(), f->timeTemps[j].epD().eepromAddr(), f->timeTemps[i].epD().getDataSize());             
}

//void TT_EpMan::sortTTs(U1_ind dp, U1_ind tt){ // insert into sorted list, in increasing order of time
//	U1_ind insertPos = f->dailyPrograms[dp].getVal(E_firstTT);
//	U1_ind lastTT = insertPos + f->dailyPrograms[dp].getVal(E_noOfTTs);
//	U1_byte ttTime = f->timeTemps[tt].getVal(E_ttTime);
//	// Recycle TT if already exists
//	for (U1_count i = insertPos; i < lastTT; ++i) {
//		if (i != tt && ttTime == f->timeTemps[i].getVal(E_ttTime)) {
//			recycle(tt);
//			ttTime = SPARE_TT;
//			break;
//		}
//	}
//	// Find new position for tt
//	while (insertPos < lastTT && insertPos != tt && ttTime >= f->timeTemps[insertPos].getVal(E_ttTime))
//		++insertPos;
//	if (insertPos != tt) {
//		U1_byte ttTemp = f->timeTemps[tt].getVal(E_ttTemp);
//		if (insertPos > tt )
//			moveTTs(tt, tt+1, insertPos-tt-1);
//		else
//			moveTTs(tt-1, tt, tt-insertPos);
//		f->timeTemps[insertPos].setVal(E_ttTime,ttTime);
//		f->timeTemps[insertPos].setVal(E_ttTemp,ttTemp);
//	}
//}

//void TT_EpMan::moveTTs(U1_ind startPos, U1_ind destination, U1_count noToMove) {
//	copyWithinEEPROM(f->timeTemps.startAddr() + f->timeTemps.dataSize() * startPos,
//		f->timeTemps.startAddr() + f->timeTemps.dataSize() * destination,
//		f->timeTemps.dataSize() * noToMove);
//}

void TT_EpMan::sortTTs(U1_ind dp) {
	U1_ind firstTT = f->dailyPrograms[dp].getVal(E_firstTT);
	U1_count noOfTTs = f->dailyPrograms[dp].getVal(E_noOfTTs);
	U1_ind lastTT = firstTT + noOfTTs - 1;
	// bubble sort TTs in increasing order of time
	for(U1_ind i=0; i < noOfTTs; i++) {
		for(U1_ind j=firstTT; j < lastTT-i; j++) {
			U1_byte firstT = f->timeTemps[j].getVal(E_ttTime);
			U1_byte secondT = f->timeTemps[j+1].getVal(E_ttTime);
			if(firstT == secondT && firstT != SPARE_TT) {
				recycle(j+1);
				if (i > 0) --i;
			}
			if(firstT > secondT)
				swapTTs(j, j+1);
		}    
	}
}

/*
int cycleSort(int *array,int size) { // Sort with minimum writes to EEPROM.
    int writes = 0;
    int cycleStart;
    for (cycleStart=0; cycleStart<size - 1;cycleStart++)
    {
        int item = array[cycleStart];

        int pos = cycleStart,i;
        for (i=cycleStart + 1;i< size;i++)
            if (array[i] < item)
                pos += 1;

        if (pos == cycleStart)
            continue;

        while (item == array[pos])
            pos += 1;
        array[pos] ^= item; // xor
        item ^= array[pos];
        array[pos] ^= item;
        writes += 1;

        while (pos != cycleStart)
        {

            pos = cycleStart;
            for (i=cycleStart + 1;i< size;i++)
                if (array[i] < item)
                    pos += 1;

            while (item == array[pos])
                pos += 1;
            array[pos]^= item;
            item^=array[pos];
            array[pos]^= item;
            writes += 1;
        }
    }
    return writes;
}
*/

U1_ind TT_EpMan::getInsertPos(S2_byte gap, U1_ind myDP, U1_ind insertPos) {
	S1_byte noOfTTs = f->dailyPrograms[myDP].getVal(E_noOfTTs);
	if (gap >= 0) f->dailyPrograms[myDP].setVal(E_noOfTTs,noOfTTs+1);	
	if (noOfTTs == 0) { // no TT's for this DP, they can start at the gap
		insertPos = U1_ind(gap);
		f->dailyPrograms[myDP].setVal(E_firstTT,insertPos);
	} else 
		insertPos = f->dailyPrograms[myDP].getVal(E_firstTT) + noOfTTs;
	return insertPos;
}

void TT_EpMan::moveData(U1_ind startPos, U1_ind destination, U1_byte noToCopy) {
	f->timeTemps.moveData(startPos, destination, noToCopy);
}

void TT_EpMan::modifyReferences(U1_ind startPos, U1_ind destination, S1_ind modify) {
	U1_byte gotNoOfZones = f->numberOf[0].getVal(noZones);
	U1_byte z = 0;
	do {
		U1_byte d = f->zones[z].getVal(E_firstDP);
		U1_byte gotLastDP = d + f->zones[z].getVal(E_NoOfDPs);
		do {
			U1_byte gotFirstTT = f->dailyPrograms[d].getVal(E_firstTT);
			if (gotFirstTT >= startPos && gotFirstTT <= destination && f->dailyPrograms[d].getVal(E_dpType) < E_dpDayOff) {
				#if defined (DEBUG_INFO)
					std::cout << "First TT: " << int(gotFirstTT) << " -> " << int(gotFirstTT + modify) << " Time: " << (int)f->timeTemps[gotFirstTT + modify].getVal(E_ttTime) << '\n';
				#endif
				f->dailyPrograms[d].setVal(E_firstTT, gotFirstTT + modify);
			}
		} while (++d < gotLastDP);
//		f->zoneR(z).refreshCurrentTT(*f);
	} while (++z < gotNoOfZones);
}

void TT_EpMan::initializeNewItem(U1_ind insertPos) {
	f->timeTemps[insertPos].setVal(E_ttTime,SPARE_TT);
}

U1_ind TT_EpMan::getLocalSpareItem(U1_ind myDP) { // Returns position of last spare TT for this DP. Returns 0 if none exists.
	if (f->dailyProgS(myDP).getDPtype() != E_dpWeekly) return 0;
	U1_ind firstTT = f->dailyPrograms[myDP].getVal(E_firstTT);
	S2_byte tryTT = firstTT + f->dailyPrograms[myDP].getVal(E_noOfTTs);
	while (--tryTT >= firstTT && f->timeTemps[U1_ind(tryTT)].getVal(E_ttTime) != SPARE_TT){}
	if (tryTT < firstTT) tryTT = 0;
	return U1_ind(tryTT);
}

U1_ind TT_EpMan::findSpareItem() { // Returns position of first spare TT. Returns 0 if no space exists
	U1_ind spare = 0;
	U1_byte gotNoOfZones = f->numberOf[0].getVal(noZones);
	U1_ind tryZone = 0;
	U1_ind tryDP;
	do { // for each zone
		tryDP = f->zones[tryZone].getVal(E_firstDP);
		U1_ind gotLastDP = tryDP + f->zones[tryZone].getVal(E_NoOfDPs);
		do  { // for each DP in this zone look for recycled TT's
			spare = getLocalSpareItem(tryDP);
		} while (spare == 0 && ++tryDP < gotLastDP);
	} while (spare == 0 && ++tryZone < gotNoOfZones);

	if (spare != 0) { // reduce TT count on DP providing spare TT
		f->dailyPrograms[tryDP].setVal(E_noOfTTs, f->dailyPrograms[tryDP].getVal(E_noOfTTs) - 1);
	}
	return spare;
}

U1_ind TT_EpMan::createNewItem() {
	logToSD("TT_EpMan::createNewItem()");
	U1_ind currDT = Zone_Stream::currZdtData.currDT;
	U1_ind newInd = f->numberOf[0].getVal(noAllTTs) +1;
	if (f->timeTemps.startAddr() + (f->timeTemps.dataSize() * newInd) <= EEPROMtop) {
		f->numberOf[0].setVal(noAllTTs, newInd);
		thisObject = &f->refreshTTs()[0].epM();
		--newInd;
	} else newInd = 0;
	Zone_Stream::currZdtData.currDT = currDT; // refresh messes with currDT
	return newInd;
}

#if defined (DEBUG_INFO)
void debugTTs(char* msg) {
	std::cout  << msg << '\n';
	for (int i = 0;i<getFactory()->numberOf[0].getVal(noAllTTs);++i){
//		std::cout  << "TT[" << std::setw(2) << int(i) << "] " << (int)getFactory()->timeTemps[i].getVal(E_ttTime)/8 << ":" <<(int)(getFactory()->timeTemps[i].getVal(E_ttTime)%8)*10 << '\n';
	}
}
#endif