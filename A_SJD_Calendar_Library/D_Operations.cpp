#include "D_Operations.h"
#include "D_EpData.h"

FactoryObjects * Run::f;

Run::Run(EpData & epD) : epDataR(epD) {}

void Run::setVal(U1_ind vIndex, U1_byte val) {epDataR.setVal(vIndex, val);}

U1_byte Run::getVal(U1_ind vIndex) const {return epDataR.getVal(vIndex);}

void Operations::setVal(U1_ind vIndex, U1_byte val) {epD().setVal(vIndex, val);}

U1_byte Operations::getVal(U1_ind vIndex) const {return epD().getVal(vIndex);}
