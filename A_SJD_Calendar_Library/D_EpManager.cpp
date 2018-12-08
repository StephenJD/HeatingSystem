#include "debgSwitch.h"
#include "D_EpManager.h"
#include "DailyProg_Stream.h"
#include "EEPROM_Utilities.h"
#include "D_Factory.h"

using namespace EEPROM_Utilities;

FactoryObjects * EpManager::f;

EpManager::EpManager() {}

EpManager * EpManager::thisObject;

U1_ind EpManager::insertNewItem(U1_ind myParent, U1_ind & originalPos, bool moveToInsertPos) { // Returns index of new Item or 0 if failed.
	// originalPos is adjusted to point to the original if it gets moved
	thisObject = this; // getNewItem() may recreate the current object, so need to be able to reassign 'this'.
	// Not allowed to reassign 'this', so save as a static pointer which can be reassigned.
	// Then call the member methods using the possibly modified static 'this'.
	U1_ind currDTPos = Zone_Stream::currZdtData.viewedDTposition;
	U1_ind insertPos = originalPos;
	S2_byte gap = getNewItem(myParent);
	Zone_Stream::currZdtData.viewedDTposition = currDTPos;
#if defined DEBUG_INFO	
	if (this != thisObject) std::cout << " Object Address Changed\n";
	debugDTs(0,"EpManager::insertNewItem gotNewItem");
#endif
	insertPos = thisObject->getInsertPos(gap, myParent, insertPos);
	
	U1_ind startPos, destination;
	S2_byte noToCopy;
	if (gap == 0) return 0; // no room
	else if (gap < 0) {// found spare in parent
		gap = -gap;
		if (!moveToInsertPos) return U1_ind(gap);
	}
	if (gap < insertPos) {
		startPos = gap+1;
		destination = U1_ind(gap);
		noToCopy = insertPos - startPos;
	} else {
		startPos = insertPos;
		destination = startPos + 1;
		noToCopy = gap - startPos;
	}

	if (noToCopy > 0) {
		thisObject->moveData(startPos,destination,U1_byte(noToCopy));
		// Move indexes in parent
		S1_ind modify;
		if (gap < insertPos) { // gap before
			destination = insertPos -1; // original pos of last moved item
			modify = -1; // how to modify references to items
			--insertPos;
			--originalPos;
		} else { // gap after
			startPos = insertPos; // original pos of first moved item
			destination = gap - 1; // original pos of last moved item
			modify = 1; // how to modify references to items
		}
		if (insertPos == originalPos) ++originalPos; // increment originalPos because new item inserted at originalPos, shifting original along by one.
		//cout << "Before modifyReferences FirstDP:" << (int)f->zones[myParent].getVal(E_firstDP) << "\n";
		thisObject->modifyReferences(startPos, destination, modify);
		//cout << "After modifyReferences FirstDP:" << (int)f->zones[myParent].getVal(E_firstDP) << "\n";
	}
	thisObject->initializeNewItem(insertPos);

	return insertPos;
}

U1_ind EpManager::getInsertPos(S2_byte gap, U1_ind myZ, U1_ind insertPos) {
	return insertPos;
}

S2_byte EpManager::getNewItem(U1_ind myParent) { // Returns position of new TT. Returns 0 if no space exists
	U1_ind newInd = getLocalSpareItem(myParent);

	if (newInd != 0)
		return -newInd;
	else {
	//	newInd = createNewItem();
	//	if (newInd > 0) {
	//		return newInd;
	//	} else {
	//		return findSpareItem();
	//	}
		newInd = findSpareItem();
		if (!newInd) {
			newInd = createNewItem();
		}
	} 
	return newInd;
}

U1_ind EpManager::makeSpaceBelow(StartAddr dataStart, U1_byte space){
	// Moves the specified block at dataStart to make space below it.
	// returns index of next object of the datatype below the one moved.
	U2_epAdrr gapAbove;
	U1_count nextDisplay = f->numberOf[0].getVal(noDisplays);
	U1_count nextRelay = f->numberOf[0].getVal(noRelays);
	U1_count nextMixingValve = f->numberOf[0].getVal(noMixValves);
	U1_count nextTempSens = f->numberOf[0].getVal(noTempSensors);
	U1_count nextOccHtr = f->numberOf[0].getVal(noOccasionalHts);
	U1_ind nextZone = f->numberOf[0].getVal(noZones)-1; // index of last zone
	U1_ind nextDT = f->zones[nextZone].getVal(E_firstDT) + f->zones[nextZone].getVal(E_NoOfDTs); // index of next free DT
	U1_ind nextDP = f->zones[nextZone].getVal(E_firstDP) + f->zones[nextZone].getVal(E_NoOfDPs);
	U1_ind nextTT = f->numberOf[0].getVal(noAllTTs);// index of next free TT

	nextZone++; // index of next free Zone

	switch (dataStart) {
	case RELAY_ST: // Move Relays
		gapAbove = f->mixingValves.startAddr() - f->relays.startAddr() - f->relays.dataSize() * nextRelay; // bytes spare above TempSens
		if (gapAbove < space){
			makeSpaceBelow(MIXVALVE_ST, space);
			gapAbove = f->mixingValves.startAddr() - f->relays.startAddr() - f->relays.dataSize() * nextRelay; // bytes spare above TempSens
		}
		if (gapAbove >= space){
			copyWithinEEPROM(f->relays.startAddr(), f->relays.startAddr() + space, f->relays.dataSize() * nextRelay);
			f->relays.setStartAddr(f->relays.startAddr() + space);
			return nextDisplay;
		} else return -1;
	case MIXVALVE_ST: // Move MixingValves
		gapAbove = f->tempSensors.startAddr() - f->mixingValves.startAddr() - f->mixingValves.dataSize() * nextMixingValve; // bytes spare above TempSens
		if (gapAbove < space){
			makeSpaceBelow(TEMP_SNS_ST, space);
			gapAbove = f->tempSensors.startAddr() - f->mixingValves.startAddr() - f->mixingValves.dataSize() * nextMixingValve; // bytes spare above TempSens
		}
		if (gapAbove >= space){
			copyWithinEEPROM(f->mixingValves.startAddr(), f->mixingValves.startAddr() + space, f->mixingValves.dataSize() * nextMixingValve);
			f->mixingValves.setStartAddr(f->mixingValves.startAddr() + space);
			return nextRelay;
		} else return -1;
	case TEMP_SNS_ST: // Move TempSens
		gapAbove = f->occasionalHeaters.startAddr() - f->tempSensors.startAddr() - f->tempSensors.dataSize() * nextTempSens; // bytes spare above TempSens
		if (gapAbove < space){
			makeSpaceBelow(OCC_HTR_ST, space);
			gapAbove = f->occasionalHeaters.startAddr() - f->tempSensors.startAddr() - f->tempSensors.dataSize() * nextTempSens; // bytes spare above TempSens
		}
		if (gapAbove >= space){
			copyWithinEEPROM(f->tempSensors.startAddr(), f->tempSensors.startAddr() + space, f->tempSensors.dataSize() * nextTempSens);
			f->tempSensors.setStartAddr(f->tempSensors.startAddr() + space);
			return nextMixingValve;
		} else return -1;

	case OCC_HTR_ST: // Move OccHtrs
		gapAbove = f->zones.startAddr() - f->occasionalHeaters.startAddr() - f->occasionalHeaters.dataSize() * nextOccHtr; // bytes spare above OccHtrs
		if (gapAbove < space){
			makeSpaceBelow(ZONE_ST, space);
			gapAbove = f->zones.startAddr() - f->occasionalHeaters.startAddr() - f->occasionalHeaters.dataSize() * nextOccHtr; // bytes spare above OccHtrs
		}
		if (gapAbove >= space){
			copyWithinEEPROM(f->occasionalHeaters.startAddr(), f->occasionalHeaters.startAddr() + space, f->occasionalHeaters.dataSize() * nextOccHtr);
			f->occasionalHeaters.setStartAddr(f->occasionalHeaters.startAddr() + space);
			return nextTempSens;
		} else return -1;
	case ZONE_ST: // Move Zones
		gapAbove = f->dateTimes.startAddr() - f->zones.startAddr() - f->zones.dataSize() * nextZone; // bytes spare above Zones
		if (gapAbove < space){
			makeSpaceBelow(DATE_TIME_ST, space);
			gapAbove = f->dateTimes.startAddr() - f->zones.startAddr() - f->zones.dataSize() * nextZone; // bytes spare above Zones
		}
		if (gapAbove >= space){
			copyWithinEEPROM(f->zones.startAddr(), f->zones.startAddr() + space, f->zones.dataSize() * nextZone);
			f->zones.setStartAddr(f->zones.startAddr() + space);
			return nextOccHtr;
		} else return -1;
	case DATE_TIME_ST: // Move DTs
		gapAbove = f->dailyPrograms.startAddr() - f->dateTimes.startAddr() - f->dateTimes.dataSize() * nextDT; // bytes spare above DT's
		if (gapAbove < space){
			makeSpaceBelow(DAYLY_PROG_ST, space);
			gapAbove = f->dailyPrograms.startAddr() - f->dateTimes.startAddr() - f->dateTimes.dataSize() * nextDT; // bytes spare above DT's
		}
		if (gapAbove >= space){
			copyWithinEEPROM(f->dateTimes.startAddr(), f->dateTimes.startAddr() + space, f->dateTimes.dataSize() * nextDT);
			f->dateTimes.setStartAddr(f->dateTimes.startAddr() + space);
			return nextZone;
		} else return -1;
	case DAYLY_PROG_ST: {// Move DPs
		U2_epAdrr ttStart = f->timeTemps.startAddr();
		U2_epAdrr dpStart = f->dailyPrograms.startAddr();
		U2_epAdrr dpSize = f->dailyPrograms.dataSize();
		gapAbove =  f->timeTemps.startAddr() - f->dailyPrograms.startAddr() - f->dailyPrograms.dataSize() * nextDP; // bytes spare above DP's
		if (gapAbove < space){
			makeSpaceBelow(TIME_TEMP_ST, space);
			gapAbove =  f->timeTemps.startAddr() - f->dailyPrograms.startAddr() - f->dailyPrograms.dataSize() * nextDP; // bytes spare above DP's
		}
		if (gapAbove >= space){
#if defined (DEBUG_INFO)
std::cout << "\nMove All DPs: " << (int) f->dailyPrograms.startAddr() << " to: " << (int)f->dailyPrograms.startAddr() + space << " Size: " << int(f->dailyPrograms.dataSize() * nextDP) << '\n';
std::cout << " Pre-Move:  DP[0]: " << f->dailyProgS(0).getName() << " DP[" << int(nextDP-1) << "]: " << f->dailyProgS(nextDP-1).getName() << '\n';
#endif
			copyWithinEEPROM(f->dailyPrograms.startAddr(), f->dailyPrograms.startAddr() + space, f->dailyPrograms.dataSize() * nextDP);
			f->dailyPrograms.setStartAddr(f->dailyPrograms.startAddr() + space);
#if defined (DEBUG_INFO)
std::cout << " Post-Move: DP[0]: " << f->dailyProgS(0).getName() << " DP[" << int(nextDP-1) << "]: " << f->dailyProgS(nextDP-1).getName() << '\n';
#endif
			return nextDT;
		} else return -1;
						}
	case TIME_TEMP_ST: // Move TTs
		gapAbove = EEPROMtop - f->timeTemps.startAddr() - f->timeTemps.dataSize() * nextTT; // bytes spare above TT's
		if (gapAbove >= space){
//std::cout << "\nMove All TTs: " << (int) f->timeTemps.startAddr() << " to: " << (int)f->timeTemps.startAddr() + space;
//std::cout << " FirstTT Time: " << (int)f->timeTemps[0].getVal(E_ttTime) << " TT[16] Time: " << (int)f->timeTemps[16].getVal(E_ttTime) << '\n';
			copyWithinEEPROM(f->timeTemps.startAddr(), f->timeTemps.startAddr() + space, f->timeTemps.dataSize() * nextTT);
			
			f->timeTemps.setStartAddr(f->timeTemps.startAddr() + space);
//std::cout << "FirstTT Time: " << (int)f->timeTemps[0].getVal(E_ttTime) << " TT[16] Time: " << (int)f->timeTemps[16].getVal(E_ttTime) << "\n\n";
			return nextDP;
		} else return -1;
	}
	return -1;
}