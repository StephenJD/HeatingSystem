#pragma once
////////////////// typedefs /////////////////////
// All types, classes, enums and namespaces have initial capital.
// U_ == unsigned, S_ == signed, B_ == boolean, E_ == enumeration, C_ == command enum, T_ == template type placeholder.
// 1,2,4 == no of bytes.

typedef unsigned char U1_count; // unsigned byte, general purpose count
typedef unsigned char U1_ind; // unsigned index
typedef unsigned char U1_byte; // general purpose
typedef unsigned short U2_byte; // general purpose
typedef unsigned short U2_epAdrr; // EEPROM address
typedef unsigned long U4_byte; // general purpose

typedef signed char S_char; // signed char, character
typedef signed char S1_byte; // signed char, general purpose
typedef signed char S1_err; // signed error code
typedef signed short S2_byte; // signed short, general purpose
typedef signed char S1_ind; // signed byte, index
typedef signed char S1_inc; // signed byte, increment
typedef signed char S1_newPage; // signed byte, new page no
typedef long S4_byte; // general purpose

typedef bool B_to_next_fld; // bool, cursor moved to next field
typedef bool B_fld_isSel; // bool, field is selected
typedef bool B_fld_desel; // bool, field deselected
typedef bool B_selectable; // bool, field selectable