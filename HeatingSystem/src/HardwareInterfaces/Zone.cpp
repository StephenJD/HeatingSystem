#include "Zone.h"


namespace HardwareInterfaces {

	//***************************************************
	//              Zone Dynamic Class
	//***************************************************

	Zone::Zone(int tempReq, int currTemp)
		: _currProgTempRequest(tempReq),
		_currTempRequest(tempReq),
		_currTemp(currTemp),
		_isHeating(tempReq > currTemp ? true : false)
	{}
}