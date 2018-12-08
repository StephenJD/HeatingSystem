#include "debgSwitch.h"
#include "D_Factory.h"
#include "DailyProg_Stream.h"
#include "EEPROM_Utilities.h"
#include "TimeTemp_EpMan.h"
#include "Zone_Run.h"

#if defined (ZPSIM)
	#include <iostream>
	#include <string>
	using namespace std;
#endif

using namespace EEPROM_Utilities;

U1_ind DP_EpMan::copyItem(U1_ind zone, U1_ind & copyFrom, bool noAdoption) { // insert new DP at Copy Index. This means we will point to the newly created DP.
	// If copyFrom is a E_dpNew with E_days == 0 it can be adopted and re-named, otherwise a copy must be made.
	// If copyFrom is not-a-E_dpNew and has a refcount of 0, it can be adopted unless noAdoption argument is true, otherwise a copy must be made.

#if defined DEBUG_INFO
	U1_ind debug = copyFrom;
	std::cout << "\nCopyDP requested from " << f->dailyProgS(copyFrom).getName() << " at " << (int) copyFrom << " for Z " << (int)zone <<'\n';
	cout << debugDPs("");
#endif

	U1_ind newInd(0);
	E_dpTypes fromType = f->dailyProgS(copyFrom).getDPtype();
	if (fromType == E_dpNew) { // adopt or copy "New Prog"
		if (f->dailyProgS(copyFrom).getVal(E_days) == 0) { // truly spare
			newInd = copyFrom; // doesn't need moving
			f->dailyProgS(newInd).setVal(E_days,255);
		}
	} else {
		if (f->dailyProgS(copyFrom).refCount() == 0 && !noAdoption) {// adopt unused named DP
			newInd = copyFrom; // doesn't need moving
		}
	}
	
	if (newInd == 0) {// need to insert a new DP
int debug = copyFrom;
		newInd = f->dailyPrograms[0].epM().insertNewItem(zone,copyFrom,noAdoption);

		if (newInd != 0) { // copy DP data
			#if defined (DEBUG_INFO)
				std::cout << "Given new DP at: " << (int)newInd << " for: " << f->dailyProgS(copyFrom).getName() << " now at: " << (int) copyFrom << " NoOfTTs: " << (int)f->dailyPrograms[newInd].getVal(E_noOfTTs) << '\n';
				cout << "FirstDP:" << (int)f->zones[zone].getVal(E_firstDP) << "\n";
				cout << debugDPs("");
			#endif
if (debug != copyFrom) { 
	debug = debug;
}
			f->dailyProgS(newInd).setVal(E_days,f->dailyPrograms[copyFrom].getVal(E_days));
			f->dailyProgS(newInd).setName(f->dailyProgS(copyFrom).getName());
			f->dailyProgS(newInd).setVal(E_dpType,f->dailyProgS(copyFrom).getDPtype()); // sets refcount to 0

			if (f->dailyProgS(newInd).getDPtype() == E_dpWeekly) {// get/copy TT's
				U1_ind copyTT = f->dailyPrograms[copyFrom].getVal(E_firstTT);
				U1_count noToCopy = f->dailyPrograms[newInd].getVal(E_noOfTTs);
				U1_count i = 0;
				U1_ind newTT = TT_EpMan::copyItem(newInd,0,0);
				if (newTT) {
					while (++i <= noToCopy) {
						f->timeTemps[newTT].setVal(E_ttTime,f->timeTemps[copyTT].getVal(E_ttTime));
						f->timeTemps[newTT].setVal(E_ttTemp,f->timeTemps[copyTT].getVal(E_ttTemp));
						++copyTT;
						if (i < noToCopy) {
							newTT = TT_EpMan::copyItem(newInd,i-1,0);
							if (newTT > 0);
							else i = noToCopy;
						}
					}
				}
			}
		}
	}
	return newInd;
}

//bool DP_EpMan::onlyOneDay(U1_ind dp){
//	U1_count noOfDays = 0;
//	U1_byte days = f->dailyProgS(dp).getDays();
//	while (days > 0 && noOfDays == 0){
//		noOfDays = noOfDays + (days % 2);
//		days = days>>1;
//	}
//	return noOfDays == 1;
//}
U1_ind DP_EpMan::findZoneForDP(U1_ind dpIndex){
	U1_byte gotNoOfZones = f->numberOf[0].getVal(noZones);
	U1_ind tryZone = 0, tryDP,lastDP;
	do { // for each zone
		tryDP = f->zones[tryZone].getVal(E_firstDP);
		lastDP = tryDP + f->zones[tryZone].getVal(E_NoOfDPs);
	} while ((dpIndex < tryDP || dpIndex >= lastDP) && ++tryZone < gotNoOfZones);
	return tryZone;
}

void DP_EpMan::recycleDP(U1_ind zIndex, U1_ind dpIndex){
	// set DP properties
	char recycleName[DLYPRG_N+1];
	strcpy(recycleName,f->dailyProgS(dpIndex).getName());
	f->dailyProgS(dpIndex).setVal(E_dpType,f->dailyProgS(dpIndex).getDPtype()); // sets refcount to 0
	if (recycleName[0] == ' ') {
		// recycle TT's and give to owner of previous TT.
		if (f->dailyProgS(dpIndex).getDPtype() == E_dpWeekly) {
			U1_ind thisTT = f->dailyProgS(dpIndex).getVal(E_firstTT);
			U1_ind noOfTTs = f->dailyProgS(dpIndex).getVal(E_noOfTTs);
			U1_ind prevDP = recipientOfTT(thisTT);
			f->dailyPrograms[prevDP].setVal(E_noOfTTs,f->dailyPrograms[prevDP].getVal(E_noOfTTs) + noOfTTs);
			U1_ind lastTT = thisTT + noOfTTs;
			for (;thisTT < lastTT; ++thisTT) {
				TT_EpMan::recycle(thisTT);
			}
			f->dailyProgS(dpIndex).setVal(E_firstTT,0);
			f->dailyProgS(dpIndex).setVal(E_noOfTTs,0);
		}
		f->dailyProgS(dpIndex).setName(NEW_DP_NAME);
		f->dailyProgS(dpIndex).setVal(E_dpType,E_dpNew);
		f->dailyProgS(dpIndex).setVal(E_days,255);
		strcpy(recycleName,NEW_DP_NAME);
	}
	// Look for another DP with this name
	if (zIndex == 255) zIndex = findZoneForDP(dpIndex);
	U1_ind thisDP = f->zones[zIndex].getVal(E_firstDP);
	U1_ind lastDP = thisDP + f->zones[zIndex].getVal(E_NoOfDPs);
	U1_ind tryDP = thisDP;
	while (tryDP == dpIndex || (tryDP < lastDP && strcmp(recycleName,f->dailyProgS(tryDP).getName()) != 0)) {
		++tryDP;
	}

	if (tryDP < lastDP) { // tryDP points to first DP of same name, so can make this really spare
		f->dailyProgS(dpIndex).setName(NEW_DP_NAME);
		f->dailyProgS(dpIndex).setVal(E_dpType,E_dpNew);
		f->dailyProgS(dpIndex).setVal(E_days,0); // indicates truly spare
	}

#if defined DEBUG_INFO
		std::cout << "\nRecycled DP[" << (int)dpIndex << "] to :" << f->dailyProgS(dpIndex).getName() << " RefCount: " << (int)f->dailyProgS(dpIndex).refCount() << "\n";
		cout << debugDPs("");
		debugTTs("");
#endif

}

U1_ind DP_EpMan::recipientOfTT(U1_ind thisTT) {
	U1_ind stopDP = f->numberOf[0].getVal(noAllDPs);
	for (U1_ind i = 0; i < stopDP; ++i) {
		if (f->dailyPrograms[i].getVal(E_firstTT) + f->dailyPrograms[i].getVal(E_noOfTTs) == thisTT && f->dailyProgS(i).getDPtype() == E_dpWeekly)
			return i;
	}
	return 0;
}

void DP_EpMan::sortDPs(U1_ind dpGroupStartIndex) { //Sorts DPs in day order.
	U1_ind endGroup = dpGroupStartIndex;
	while (!f->dailyProgS(endGroup).endOfGroup()) {++endGroup;}
	U1_count noInGroup = endGroup - dpGroupStartIndex;
	// bubble sort DPs in decreasing order of byte-value
	for(U1_ind i=0; i <= noInGroup; i++) {
		for(U1_ind j=dpGroupStartIndex; j < endGroup-i; j++) {
			if(f->dailyProgS(j).getDays() < f->dailyProgS(j+1).getDays()) {
				swapDPs(j, j+1);
			}   
		}    
	}
	resetEndOfGroup(dpGroupStartIndex);
}

void DP_EpMan::swapDPs(U1_ind i, U1_ind j) {
	swapData(f->dailyPrograms[i].epD().eepromAddr(), f->dailyPrograms[j].epD().eepromAddr(), f->dailyPrograms[i].epD().getDataSize());             
}

U1_ind DP_EpMan::startOfGroup(U1_ind dpIndex) { // returns index of first weekly DP in current group.
	// Move backwards to first DP of same name
	while (dpIndex > 0 && !f->dailyProgS(dpIndex-1).endOfGroup())
		--dpIndex;
	return dpIndex;
}

U1_ind DP_EpMan::lastInGroup(U1_ind dpIndex) { // returns index of last weekly DP in current group.
	if (f->dailyProgS(dpIndex).getDPtype() != E_dpWeekly) return dpIndex;
	U1_count refCt = f->dailyProgS(dpIndex).refCount();
	char thisName[DLYPRG_N+1];
	strcpy(thisName,f->dailyProgS(dpIndex).getName());
	do {
		++dpIndex;
	} while (f->dailyProgS(dpIndex).refCount() == refCt && strcmp(f->dailyProgS(dpIndex).getName(),thisName) == 0);
	return --dpIndex;
}

void DP_EpMan::resetEndOfGroup(U1_ind dpGroupStartIndex) {
	U1_byte daysSoFar = 0;
	do {
		daysSoFar = daysSoFar | f->dailyProgS(dpGroupStartIndex).getDays();
		if (daysSoFar < ALL_DAYS)
			f->dailyProgS(dpGroupStartIndex++).endOfGroup(false);
		else
			f->dailyProgS(dpGroupStartIndex).endOfGroup(true);
	} while (daysSoFar < ALL_DAYS);
}

S1_inc DP_EpMan::removeDuplicateDays(U1_ind zIndex, const U1_ind dpIndex) { // returns offset from dpIndex for the DP containing the days we are showing
	// Duplicates are created by adding days to this DP, in which case we remove the duplicates from some other DP.
	// This might result in empty DP's which are recycled.
	// Days removed from this DP are added to the next (poss new) DP.
	// On removing all the days, we return an offset to the DP now holding that day.
	const U1_ind startGroup = startOfGroup(dpIndex);
	const U1_ind endGroup = lastInGroup(dpIndex);
	// Get days in this group

	U1_byte interestingDays = f->dailyProgS(dpIndex).getDays();
	U1_byte myDays = interestingDays;
	// Remove them from other DP's.
	U1_ind dp = startGroup;
	while (interestingDays) {
		if (dp == dpIndex) ++dp; // need to stop at end of group!
		if (dp > endGroup) break;
		U1_byte thisDPdays = f->dailyProgS(dp).getDays();
		f->dailyProgS(dp).setDays(thisDPdays & ~interestingDays);
		interestingDays = interestingDays & ~thisDPdays;
		++dp;
	}

	// Add the missing days to a new DP.
	interestingDays = missingDays(startGroup);
	//S1_inc returnVal = 0;
	if (interestingDays != 0) {
		dp = dpIndex;
		if (f->dailyProgS(dpIndex).getDays() == 0) { // if we have emptied current DP, add days to next DP
			if (f->dailyProgS(dpIndex).endOfGroup()) { 
				if (dpIndex > startGroup) {
					--dp;
					//returnVal = -1;
				}
			} else {
				++dp;
			}
			if (dp != dpIndex) recycleDP(zIndex, dpIndex);
			f->dailyProgS(dp).setDays(f->dailyProgS(dp).getDays() | interestingDays);
		} else { // not emptied the DP so add missing day to a new DP
			U1_count refs = f->dailyProgS(dpIndex).refCount();
			U1_ind originalDP = dpIndex;
			dp = copyItem(zIndex, originalDP,true);
			f->dailyProgS(dp).refCount(refs);
			f->dailyProgS(dp).setDays(interestingDays);
		}
		resetEndOfGroup(startGroup);
	}
	// remove empty DPs
	bool doAgain = true;
	dp = startGroup;
	do {
		if (!f->dailyProgS(dp).getDays()){ // no days set, so recycle DP
			if (dp == startGroup) { // copy ref count to next DP
				f->dailyProgS(dp+1).refCount(f->dailyProgS(dp).refCount());
			}	
			if (f->dailyProgS(dp).endOfGroup()) {
				f->dailyProgS(dp-1).endOfGroup(true);
				doAgain = false;
			}
			recycleDP(zIndex,dp);
			dp++;
		} else {
			doAgain = !f->dailyProgS(dp).endOfGroup();
			dp++;
		}
	} while (doAgain);
	sortDPs(startGroup);
	S1_inc returnVal = 0;
	if (myDays != 0) {
		U1_ind i = startGroup;
		while (f->dailyProgS(i).getDays() != myDays) {++i;}
		returnVal = i-dpIndex;
	} else returnVal = -1;
	return returnVal;
}

U1_byte DP_EpMan::missingDays(U1_ind dpGroupStartIndex) { // returns byte of missing days from group.
	U1_byte daysSoFar = 0;
	do {daysSoFar = daysSoFar | f->dailyProgS(dpGroupStartIndex).getDays();
	} while (!f->dailyProgS(dpGroupStartIndex++).endOfGroup());
	return (~daysSoFar & ALL_DAYS);
}

U1_ind DP_EpMan::getInsertPos(S2_byte gap, U1_ind myZ, U1_ind insertPos) {
	S1_byte noOfDPs = f->zones[myZ].getVal(E_NoOfDPs);
	if (gap >= 0) f->zones[myZ].setVal(E_NoOfDPs,noOfDPs+1);
	if (noOfDPs == 0){
		insertPos = U1_ind(gap);
		f->zones[myZ].setVal(E_firstDP,insertPos);
	}
	return insertPos;
}

void DP_EpMan::moveData(U1_ind startPos, U1_ind destination, U1_byte noToCopy) {
	f->dailyPrograms.moveData(startPos, destination, noToCopy);
}

void DP_EpMan::modifyReferences(U1_ind startPos, U1_ind destination, S1_ind modify) {
	U1_byte gotNoOfZones = f->numberOf[0].getVal(noZones);
	U1_byte z = 0;
	do {
		// Modify firstDP Indexes
		U1_byte gotFirstDP = f->zones[z].getVal(E_firstDP);
		if (gotFirstDP >= startPos && gotFirstDP <= destination) {
			#if defined (DEBUG_INFO)
				std::cout << "Zone[" << (int)z << "] First DP: " << int(gotFirstDP) << " -> " << int(gotFirstDP + modify) << '\n';
			#endif
			f->zones[z].setVal(E_firstDP, gotFirstDP + modify);
		}
	
		// Modify DT's DP Indexes
		U1_byte gotFirstDT = f->zones[z].getVal(E_firstDT);
		U1_byte gotLastDT = gotFirstDT + f->zones[z].getVal(E_NoOfDTs);
		for (U1_ind dt = gotFirstDT; dt < gotLastDT; dt++) {
			U1_byte dpInd = f->dateTimesS(dt).dpIndex();
			if (dpInd >= startPos && dpInd <= destination) {
				#if defined (DEBUG_INFO)
					std::cout << "   DT[" << (int) dt << "] DP Index: " << int(dpInd) << " -> " << int(dpInd + modify) << '\n';
				#endif
				f->dateTimesS(dt).moveDPindex(dpInd + modify);
			}
		}
	} while (++z < gotNoOfZones);
	if (Zone_Stream::currZdtData.currDP >= startPos && Zone_Stream::currZdtData.currDP <= destination) {
		Zone_Stream::currZdtData.currDP += modify;
	}
}

void DP_EpMan::initializeNewItem(U1_ind insertPos) {
	f->dailyPrograms[insertPos].setVal(E_noOfTTs,0);
}

U1_ind DP_EpMan::getLocalSpareItem(U1_ind myZ) { // Returns position of first spare DP for this zone. Returns 0 if none exists.
	S2_byte tryDP = f->zones[myZ].getVal(E_firstDP);
	U1_ind lastDP = tryDP + f->zones[myZ].getVal(E_NoOfDPs);
	// Every Zone keeps one spare DP;
	while (tryDP < lastDP && f->dailyProgS(U1_ind(tryDP)).getDays() != 0){++tryDP;};
	if (tryDP == lastDP) tryDP = 0;
	return U1_ind(tryDP);
}

U1_ind DP_EpMan::findSpareItem() { // Returns position of first spare DP. Returns 0 if no space exists
	U1_ind spare = 0;
	U1_byte gotNoOfZones = f->numberOf[0].getVal(noZones);
	U1_ind tryZone = 0;
	U1_ind tryDP;
	do { // for each zone
		tryDP = f->zones[tryZone].getVal(E_firstDP);
		//U1_ind gotLastDP = tryDP + f->zones[tryZone].getVal(E_NoOfDPs);
		spare = getLocalSpareItem(tryZone);
	} while (spare == 0 && ++tryZone < gotNoOfZones);

	if (spare != 0) { // reduce DP count on Zone providing spare DP
		f->zones[tryZone].setVal(E_NoOfDPs, f->zones[tryZone].getVal(E_NoOfDPs) - 1);
	}
	return spare;
}

U1_ind DP_EpMan::createNewItem() {
	logToSD("DP_EpMan::createNewItem()");
	U1_ind newInd = makeSpaceBelow(TIME_TEMP_ST,f->dailyPrograms.dataSize());
	if (newInd > 0) {
		newInd = f->numberOf[0].getVal(noAllDPs);
		f->numberOf[0].setVal(noAllDPs, newInd + 1);
		thisObject = &f->refreshDPs()[0].epM();
	}
	return newInd;
}