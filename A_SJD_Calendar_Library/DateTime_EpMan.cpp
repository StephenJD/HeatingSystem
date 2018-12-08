#include "debgSwitch.h"
#include "DateTime_EpMan.h"
#include "D_Factory.h"
#include "DailyProg_Stream.h"

FactoryObjects * getFactory(FactoryObjects * fact);

#if defined (ZPSIM)
//#include <iomanip>
void debugDTs(U1_ind zone, char* msg){
	FactoryObjects & fact = *getFactory();
	std::cout  << msg << '\n';
	std::cout << std::dec << "Zone[" << (int)zone << "] DT's: " << (int)fact.zones[zone].getVal(E_NoOfDTs) << " CurrDT_pos:" << (int)Zone_Stream::currZdtData.viewedDTposition << '\n';
	//			  DT[ 0]              Now DP[0] DS-Wk   1SpareDT		
	std::cout << "        Start Date-Time     DP Name  Refs\n";
	for (int i = 0;i<fact.numberOf[0].getVal(noAllDTs);++i){
		//std::cout  << "DT[" << std::setw(2) << int(i) << "] " << std::setw(16) << 
		//	DateTime_Stream::getTimeDateStr(fact.dateTimesS(i).getDateTime());
		//std::cout << " DP[" << (int)fact.dateTimesS(i).dpIndex() << "] "<< std::setw(10) << fact.dailyProgS(fact.dateTimesS(i).dpIndex()).getName();
		//std::cout << std::setw(4) << (int)fact.dailyProgS(fact.dateTimesS(i).dpIndex()).refCount() << (fact.dateTimesS(i).getMnth() == 0 ? "SpareDT" : "") << '\n';
	}
	std::cout << "debugDTs Done.\n";
}
#endif

U1_ind DT_EpMan::copyItem(U1_ind myZ, U1_ind & copyFrom) {
U1_ind original = copyFrom;
// copy copyFrom arg because it may come from currZdtData which can get changed.
#if defined DEBUG_INFO
		U1_ind debugZone = myZ;
		std::cout << "\nCopyDT requested from DT[" << int(copyFrom) << "] for Z " << (int)myZ;
		std::cout << " has NoOfDTs: " << (int)f->zones[myZ].getVal(E_NoOfDTs) << " FirstDT: "<< (int)f->zones[myZ].getVal(E_firstDT) << '\n';
		debugDTs(myZ,"\nDT_EpMan::copyItem start");
#endif
	DTtype copyDate = f->dateTimesS(original);
	int copyDPindex = f->dateTimesS(original).dpIndex();
int debug = original;
	U1_ind newInd = f->dateTimes[0].epM().insertNewItem(myZ, original);
if (debug != original) 
	debug = debug;

	if (newInd != 0) { // copy DP data
		U1_ind dpIndex = f->dateTimesS(newInd).dpIndex();
		f->dailyProgS(dpIndex).incrementRefCount(1);
		f->dateTimesS(newInd).copy(f->dateTimesS(original));
	}
	#if defined (DEBUG_INFO)	
		std::cout << "Given new DT at: " << (int)newInd << " DPindex: " << (int)f->dateTimesS(newInd).dpIndex() << " DT original now at:" << (int)original << '\n';
		debugDTs(myZ,"\nDT_EpMan::copyItem done\n");
	#endif
    copyFrom = original;
	return newInd;
}

U1_ind DT_EpMan::getInsertPos(S2_byte gap, U1_ind myZ, U1_ind insertPos) {
	S1_byte noOfDTs = f->zones[myZ].getVal(E_NoOfDTs);
	if (gap >= 0) f->zones[myZ].setVal(E_NoOfDTs,noOfDTs+1);	
	if (noOfDTs == 0) { // no DT's for this Z, they can start at the gap
		insertPos = U1_ind(gap);
		f->zones[myZ].setVal(E_firstDT,insertPos);
	} else 
		insertPos = f->zones[myZ].getVal(E_firstDT) + noOfDTs;
	return insertPos;
}

void DT_EpMan::moveData(U1_ind startPos, U1_ind destination, U1_byte noToCopy) {	
	logToSD("DT_EpMan::moveData()");
	DateTime_Stream::saveAllDTs();
	f->dateTimes.moveData(startPos,destination,noToCopy);
	DateTime_Stream::loadAllDTs(*f);
}

void DT_EpMan::modifyReferences(U1_ind startPos, U1_ind destination, S1_ind modify) {
	U1_byte gotNoOfZones = f->numberOf[0].getVal(noZones);
	U1_byte z = 0;
	do {
		// Modify firstDT Indexes
		U1_byte gotfirstDT = f->zones[z].getVal(E_firstDT);
		if (gotfirstDT >= startPos && gotfirstDT <= destination) {
			#if defined (DEBUG_INFO)
				std::cout << "First DT: " << int(gotfirstDT) << " -> " << int(gotfirstDT + modify) << '\n';
			#endif
			f->zones[z].setVal(E_firstDT, gotfirstDT + modify);
		}
	} while (++z < gotNoOfZones);
}

void DT_EpMan::initializeNewItem(U1_ind insertPos){
	f->dateTimesS(insertPos).moveDPindex(0);
}

U1_ind DT_EpMan::getLocalSpareItem(U1_ind myZ) { // Returns position of last spare DT for this zone. Returns 0 if none exists.
	U1_ind firstDT = f->zones[myZ].getVal(E_firstDT) ;
	S2_byte tryDT = firstDT + f->zones[myZ].getVal(E_NoOfDTs);
	while (--tryDT >= firstDT && f->dateTimesS(U1_ind(tryDT)).getMnth() != 0){}
	if (tryDT < firstDT) tryDT = 0;
	return U1_ind(tryDT);
}

U1_ind DT_EpMan::findSpareItem() { // Returns position of first spare DT. Returns 0 if no space exists
	U1_ind spare = 0;
	U1_byte gotNoOfZones = f->numberOf[0].getVal(noZones);
	U1_ind tryZone = 0;
	U1_ind tryDT;
	do { // for each zone
		tryDT = f->zones[tryZone].getVal(E_firstDT);
		//U1_ind gotLastDT = tryDT + f->zones[tryZone].getVal(E_NoOfDTs);
		spare = getLocalSpareItem(tryZone);
	} while (spare == 0 && ++tryZone < gotNoOfZones);

	if (spare != 0) { // reduce DT count on Zone providing spare DT
		f->zones[tryZone].setVal(E_NoOfDTs, f->zones[tryZone].getVal(E_NoOfDTs) - 1);
	}
	return spare;
}

U1_ind DT_EpMan::createNewItem() {
	logToSD("DT_EpMan::createNewItem()");
	U1_ind newInd = makeSpaceBelow(DAYLY_PROG_ST,f->dateTimes.dataSize());	
	if (newInd > 0) {
		U1_ind lastInd = f->numberOf[0].getVal(noAllDTs);
		f->numberOf[0].setVal(noAllDTs, lastInd + 1);
		Group & newDTs = f->refreshDTs();
		thisObject = &newDTs[0].epM();
	}	
	return newInd;
}