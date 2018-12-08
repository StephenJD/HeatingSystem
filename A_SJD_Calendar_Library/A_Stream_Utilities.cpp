// Utilities.cpp: implementation of the Utilities class.
//
//////////////////////////////////////////////////////////////////////
#include "debgSwitch.h"
#include "A_Stream_Utilities.h"

char SPACE_CHAR = ' '; // temporarily here so it can be redefined by gtest

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
namespace Utils {

// 'public' objects
char result[MAX_LINE_LENGTH];
Edit * editPtr;

// 'private' objects
static char pageBuffer[DISPLAY_BUFFER_LENGTH];

#if defined (ZPSIM)
static char printBuffer[100];
#endif

S1_byte nextIndex(S1_byte minVal, S1_byte currVal, S2_byte maxVal, S1_byte move, bool carryOver){
	return static_cast<S1_byte>(nextIndex(minVal, static_cast<S2_byte>(currVal), maxVal, static_cast<S2_byte>(move), carryOver));
}

S2_byte nextIndex(S1_byte minVal, S2_byte currVal, S2_byte maxVal, S2_byte move, bool carryOver) { // return index, which has max value of noOfMembers -1
	// when incrementing by 10 or 100, leave the lower digits unchanged when overflowing the max and min values.
	S2_byte noOfMembers = maxVal - minVal +1;
	if (noOfMembers <=0) return 0;
	short absMove = abs(move);
	short power = 1;
	if (!carryOver) while (absMove / (power * 10) >= 1) power = power * 10;

	short tempVal = currVal/ power - minVal / power; // currentVal relative to minVal

	short tempNoOfMembers = (noOfMembers-1) / power + 1;
	S2_byte sum = tempVal + move/power; // new value relative to minVal
	if (sum < 0) sum =  sum + tempNoOfMembers; // cycle to end
	if (sum >= tempNoOfMembers && tempNoOfMembers >= 10) sum = sum - tempNoOfMembers; else sum = sum % tempNoOfMembers;
	sum = (minVal/power + sum) * power + (currVal % power);
	if (sum >= minVal +  noOfMembers) sum = minVal +  noOfMembers -1;
	if (sum < minVal) sum = minVal;
	return sum;	
}

char *cIntToStr(S2_byte number, U1_byte minNoOfDigits, char leadingChar, bool leadPlus){ //max 3 digits. Default is min 2 digits, leading ' ', no +
	if (minNoOfDigits > 1 && leadingChar == 0) leadingChar = ' ';
	char * debug = result;
	result[0] = leadPlus ? '+' : leadingChar; 
	result[1] = leadingChar; 
	result[2] = leadingChar; 
	result[3] = leadingChar;
	// set offset to null position
	S1_byte offset = minNoOfDigits + (leadPlus || number <0);
	// do zero and minus
	if (number == 0){
		result[offset-1]='0'; result[offset]=0;
        return result;
	}
	if (number <0){
		result[0]='-';
		number = -number;
	}
	// move to first number position
	offset = (leadPlus || number <0);
	S1_byte index = 0;
	while (minNoOfDigits - index > 5) ++index;

	// do each dec place
	static int pow10[] = {1, 10, 100, 1000, 10000, 100000};
	int p = 5;
	do {
		if (number>= pow10[p] ){
			result[index+offset] = number/pow10[p] + '0';
			number = number%pow10[p];
			++index;
			result[index+offset] = '0';
		} else if (minNoOfDigits-index > p) {
			++index;
		}
		--p;
	} while (p > 0);
	
	result[index+offset] = number + '0';
	++index;
	result[index+offset]=0; //null chr
    return result;
}

char *cDecToStr(S2_byte number, U1_byte minNoOfDigits, U1_byte noOfDecPlaces){
	cIntToStr(number,minNoOfDigits,'0');
	result[minNoOfDigits+1] = 0;
	while (noOfDecPlaces > 0) {
		result[minNoOfDigits] = result[minNoOfDigits-1];
		minNoOfDigits--;
		noOfDecPlaces--;
	}
	result[minNoOfDigits] = '.';
	return result;
}

#if defined (ZPSIM)

char * lcdToTextBox(const char * lcdBuffer) { // Used by Simulator
	// adds space at the end of each line to allow line break
	S1_byte pg_pos = 0, pt_pos = 0;
	do { 
		printBuffer[pt_pos++] = lcdBuffer[pg_pos++];
		//if (pg_pos > 0 && pg_pos % MAX_LINE_LENGTH == 0) 
		//	printBuffer[pt_pos++] = ' ';
	} while (pg_pos < DISPLAY_BUFFER_LENGTH && lcdBuffer[pg_pos] != 0); // copy to end of string
	if (printBuffer[pt_pos-1] == ' ') pt_pos--;
	printBuffer[pt_pos] = 0;
	return printBuffer;
}

char * printPage(const char * pageBuffer){ // Used by G-Test. Inserts a cursor char and cr/lf at end of each line
	// pageBuffer has cursor pos at index 0. -ve for edit, +ve for underscore, 0=off.
	// +-1 is first character position.
	//const char * debugBuffer = pageBuffer+1;
	S1_byte csrSpace = pageBuffer[0]==0? 0:1;
	S1_byte cursor_pos = abs(pageBuffer[0]) - csrSpace;
	S1_byte pg_pos = 1, pt_pos = 0;

	while (pg_pos <= cursor_pos){ // copy up to cursor
		printBuffer[pt_pos++] = pageBuffer[pg_pos++];
		if ((pg_pos-1) % 20 == 0) printBuffer[pt_pos++] = '\n';
	}

	if (pageBuffer[0]<0) 
		printBuffer[pt_pos++] = '#'; // add cursor
	else if (pageBuffer[0]>0) 
		printBuffer[pt_pos++] = '_';

	do { 
		printBuffer[pt_pos++] = pageBuffer[pg_pos++];
		if ((pg_pos-1) % 20 == 0) printBuffer[pt_pos++] = '\n';
	} while (pageBuffer[pg_pos] != 0); // copy to end of string

	do {
		if (printBuffer[pt_pos-1] == '\n') --pt_pos;
		while (printBuffer[pt_pos-1] == ' ') --pt_pos;
		while (printBuffer[pt_pos-1] == '~') --pt_pos;
	} while (printBuffer[pt_pos-1] == '\n');

	printBuffer[pt_pos] = 0;
	return printBuffer;
}
#endif

char* get_pageBuffer(){return pageBuffer;}
void clear_pageBuffer() {
	pageBuffer[0] = 0;pageBuffer[1] = 0;
	//for (S1_byte debug = 2;debug<DISPLAY_BUFFER_LENGTH;++debug) { // for testing
	//	pageBuffer[debug] = 'z';
	//}
}

bool add_to_pageBuffer(char* my_string, int rows, int cols, bool showingFirstInList){ // returns true if my_string is a list member and there is room for more
	// my_string points to single fields
	// spaces are inserted where necessary to avoid fields splitting across lines.
	if (!my_string || my_string[2]==0) return false;
	U1_byte start = strlen(pageBuffer+1);
	#if defined (ZPSIM)
		U1_byte debugCursorPos = pageBuffer[0];
	#endif
	U1_byte cursorPos = my_string[0];
	U1_byte pos = 0;
	bool noMoreOnThisLine = (my_string[1] == NEW_LINE_CHR);
	char * nextField = my_string + 2; // string without leading cursor pos or new line
	S1_byte len_next_field = strlen(nextField); // length of field without any following space
	bool is_list = (nextField[len_next_field-1] == '\t');
	static bool	haveSetLeftArrow;
	if (start == 0) haveSetLeftArrow = false;
	if (is_list) {
		len_next_field--;
		nextField[len_next_field] = SPACE_CHAR; // list members need spaces, but may already have one
	}
	bool has_space = (nextField[len_next_field-1] == SPACE_CHAR);
	if (has_space) {
		nextField[len_next_field] = 0; // make sure list members are terminated after first space
		len_next_field--;
	}
	// start indexes display, starting at 1. But on entering this 'if', start is zero, so that a field length of 20 fits.
	start++;
	if (len_next_field > 0 && start + pos + len_next_field <= (cols * rows)+1) {
		do {
			// split my_string at CONTINUATION characters, adding each substring in turn.
			S1_byte len_next_substring = 0; // length without following space
			while (len_next_substring < len_next_field && nextField[++len_next_substring] != CONTINUATION);
			
			S1_byte line_space_left = cols - (start + pos-1) % cols; // add 1 for cursor pos 
			while (line_space_left >= 0 && line_space_left < cols && (noMoreOnThisLine || line_space_left < len_next_substring)) {// if no room for next, fill rest of line with spaces >=
				pageBuffer[start + pos] = SPACE_CHAR; // add spaces to end of line
				start++;				
				line_space_left--;
			}
			noMoreOnThisLine = false;
			if (line_space_left <= 0) {
				start--;			
				line_space_left = cols - (start + pos) % cols;
			}
			if (start + pos + len_next_substring <=  cols * rows + 1) { 
				do { 
					if (!showingFirstInList && !haveSetLeftArrow) {
						pageBuffer[start++ + pos] = LIST_LEFT_CHAR;
						haveSetLeftArrow = true;
					}
					pageBuffer[start + pos] = nextField[pos]; // Add a character from our argument, incl terminating 0
					pos++;
				} while (pos <= (len_next_substring)); // copy field excluding '_' of split fields
			} 
			S1_byte cludge = 1;
			if ((start + pos) % cols ==  2) {
				start--; // remove space at start of line - 
				cludge = 0;
			} else if (pageBuffer[start + pos-1] == CONTINUATION) 
				pageBuffer[start + pos++ -1] = ' ';
			
			pageBuffer[start + pos] = 0; // terminate buffer

			nextField = nextField + len_next_substring + 1;
			len_next_field = len_next_field - len_next_substring - cludge ; //1;
			if (my_string[0] != 0)  // set cursor pos
				cursorPos = cursorPos - len_next_substring -1;

			start = start + pos - cludge; // start must point to position of next character
			pos = 0;
			#if defined (ZPSIM)
				char * debugBuffer = pageBuffer+1;
				S1_byte debugLen = start + pos;
			#endif
		} while (start + pos < cols * rows && len_next_field > 0);

		if (my_string[0] != 0)  // set cursor pos
			pageBuffer[0] = (cursorPos + start) * ((S_char)my_string[0] < 0?-1:1);

	} else {
		is_list = false; // no more room left
		pageBuffer[start + pos - 1] = LIST_RIGHT_CHAR;
		pageBuffer[start + pos] = 0;
	}
	return is_list;
}

void close_pageBuffer(bool inEdit, int rows, int cols){
	if (inEdit) pageBuffer[0] = -abs(S_char(pageBuffer[0]));
	S1_byte i;
	for (i = strlen(pageBuffer +1) + 1; i <= cols * rows; i++){
		pageBuffer[i] = ' ';
	}
	pageBuffer[i] = 0;
}

}
