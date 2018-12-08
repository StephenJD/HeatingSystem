#pragma once
#include "D_Operations.h"
#include "D_EpData.h"

class OccasHtr_Run : public RunT<EpDataT<OCCHTR_DEF> >
{
public:
	OccasHtr_Run();
	bool check(); // returns true if ON
private:
};
