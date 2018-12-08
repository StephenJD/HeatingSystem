#include "D_Group.h"
#include "D_EpDat_A.h"
#include "EEPROM_Utilities.h"

using namespace EEPROM_Utilities;

//Group::Group(U1_byte noOf, U2_epAdrr startAddr)
//	:noOfMembers(noOf),
//	dataStart(startAddr) {}

Group::Group(U1_byte noOf, Operations * first, U1_byte size)
	:noOfMembers(noOf),
	firstMember(first),
	objsize(size) 
{}

Group::~Group() {delete[] firstMember;}

//void Group::setDataStart(U2_epAdrr start){
//	dataStart = start;
//}

//const U2_epAdrr Group::getDataStart() const {return dataStart;}

const U1_byte Group::count() const {return noOfMembers;}
const U1_byte Group::last() const {return noOfMembers-1;}
const U2_epAdrr Group::startAddr() const {return firstMember->epD().getDataStart();}
const U1_byte Group::dataSize() const {return firstMember->epD().getDataSize();}
void Group::moveData(U1_ind startPos, U1_ind destination, U1_count noToMove) const {
	firstMember->epD().move(startPos,destination,noToMove);
}

void Group::setStartAddr(U2_epAdrr newAddr) {
	firstMember->epD().setDataStart(newAddr);
}

//U1_byte Group::insertBefore(U1_byte index){return 0;}
//
//U1_byte Group::pushBack(){return 0;}
//
//U1_byte Group::remove(U1_byte index){return 0;}

Operations & Group::operator [] (int index) { 
	// need to force compiler to add my increment, not use its knowledge of Operations base size
	if (index >= noOfMembers) index = 0;
	return *(reinterpret_cast<Operations *>(reinterpret_cast<long>(firstMember) + index * objsize));
}

//const Operations & Group::operator [] (int index) const {
//	return *(reinterpret_cast<Operations *>(reinterpret_cast<long>(firstMember) + index * objsize));
//}

