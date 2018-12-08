#pragma once
#include "A__Constants.h"
#include "D_DataStream.h"
#include "D_EpDat_A.h"
#include "D_EpData.h"

////////////////////////////// Events_Stream /////////////////////////////////////

class Events_Stream : public DataStreamT<EpDataT<EVENT_DEF>, RunT<EpDataT<EVENT_DEF> >, IniSet_EpMan, 1>
{
public:
	Events_Stream(RunT<EpDataT<EVENT_DEF> > & run);
	void newEvent(S1_byte errNo, S1_byte itemNo); // errNo:4 bits, itemNo:8 bits
	char * getEvent(S1_ind index);
	void clearToSerialPort();
	const U1_byte getNoOfStreamItems() const; // returns no of members of the Collection -1
private:
	char * objectFldStr(S1_ind activeInd, bool inEdit) const;
	U1_count noOfEvents;
	//U2_epAdrr nextEventAddr;
};

