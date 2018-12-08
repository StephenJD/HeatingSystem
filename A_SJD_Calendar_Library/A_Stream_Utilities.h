// A_Stream_Utilities.h: interface for the Utilities class.
//
//////////////////////////////////////////////////////////////////////
#if !defined (UTILITIES_H_)
#define UTILITIES_H_

#include "debgSwitch.h"
#include "A__Constants.h"

class Edit;

extern char SPACE_CHAR;  // temporarily here so it can be redefined by gtest
//class Edit;

namespace Utils
{
	extern char result[MAX_LINE_LENGTH]; // used to return results
	extern Edit * editPtr;

	S1_byte nextIndex(S1_byte minVal, S1_byte currVal, S2_byte maxVal, S1_byte move, bool carryOver = true);
	S2_byte nextIndex(S1_byte minVal, S2_byte currVal, S2_byte maxVal, S2_byte move, bool carryOver = true);

	// String functions
	char *cIntToStr(S2_byte number, U1_byte minNoOfDigits = 2, char leadingChar = ' ', bool leadPlus = false);
	char *cDecToStr(S2_byte number, U1_byte minNoOfDigits = 2, U1_byte noOfDecPlaces = 1);
	
	// Streaming //
	#if defined (ZPSIM)
	char * lcdToTextBox(const char * lcdBuffer);// Used by Simulator
	char * printPage(const char * pageBuffer); // Used by G-Test.
	#endif

	char* get_pageBuffer();
	bool add_to_pageBuffer(char* my_string, int rows, int cols, bool showingFirstInList = true); 	// my_string points to a single data string and returns true if was a list item 
														// spaces are inserted where necessary to avoid fields splitting across lines.
	void clear_pageBuffer();
	void close_pageBuffer(bool inEdit, int rows, int cols);
};

#endif
