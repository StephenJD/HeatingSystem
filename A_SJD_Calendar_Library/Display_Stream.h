#pragma once

#include "A__Constants.h"
#include "D_DataStream.h"
#include "D_EpData.h"

class Display_Run;
class Display_EpMan;
class Displays;

class Display_Stream : public DataStreamT<Display_EpD, Display_Run, Display_EpMan, DS_COUNT> {
public:
	Display_Stream(Display_Run & run); // , Displays * display
	~Display_Stream();
	S1_newPage onNameSelect() {return 0;}
	void setDisplay(Displays * display);
	void setBackLight(bool wake);
	static float getPhoto();
	Displays * display() {return _display;}
private: 
	Displays * _display;
};