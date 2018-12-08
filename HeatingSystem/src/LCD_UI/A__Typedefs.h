#pragma once

/////////////////////////////////////////////////////////////////////
//              Naming Convention                                  //
// Type names have All Initial Capitals  		                   //
// Function and variable names have initial lower-case             //
// Descriptive names are in CamelCase		                       //
// Name qualifiers are separated from the name with _undersore     //
// Private class-member names have initial _underscore             //
/////////////////////////////////////////////////////////////////////

////////////////// typedefs /////////////////////
// All types, classes, enums and namespaces have initial capital.
// U_ == unsigned, S_ == signed, B_ == boolean, E_ == enumeration, C_ == command enum, T_ == template type placeholder.
// 1,2,4 == no of bytes.
namespace LCD_UI {
	typedef unsigned char U1_Count; // unsigned byte, general purpose count
	typedef unsigned char U1_Ind; // unsigned index
	typedef unsigned char U1_Byte; // general purpose
	typedef unsigned short U2_Byte; // general purpose
	typedef unsigned short U2_EpAddr; // EEPROM address
	typedef unsigned int U4_Byte; // general purpose

	//typedef signed char S_Char; // signed char, character
	typedef signed char S1_Byte; // signed char, general purpose
	typedef signed char S1_Err; // signed error code
	typedef signed short S2_Byte; // signed short, general purpose
	typedef signed char S1_Ind; // signed byte, index
	typedef signed char S1_Inc; // signed byte, increment
	typedef signed char S1_NewPage; // signed byte, new page no
	typedef signed int S4_Byte; // general purpose

	typedef bool B_ToNextFld; // bool, cursor moved to next field
	typedef bool B_FldIsSel; // bool, field is selected
	typedef bool B_FldDesel; // bool, field deselected
	typedef bool B_Selectable; // bool, field selectable
}