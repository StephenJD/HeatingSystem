#pragma once
#include "d_operations.h"
#include "TowelR_Run.h"
#include "TowelR_Stream.h"

class TowelR_Operations : public Operations
{
public:
	TowelR_Operations();
	DataStream & dStream() {return serObj;}
	Run & run() {return runObj;}
	const EpData & epD() const {return runObj.epDI();}
	EpData & epD() {return runObj.epDI();}
	EpManager & epM() {return serObj.epMi();}
	void loadDefaults();

private:
	TowelR_Run runObj; // contains EpData	
	TowelR_Stream serObj;
};

