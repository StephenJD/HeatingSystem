#pragma once
#include <Arduino.h>
#include "UI_Collection.h"
#include "Convertions.h" // for client use of formatting enum

namespace LCD_UI {
	using namespace GP_LIB; // for client use of formatting enum

	class I_Field_Interface;

	// ************** valrange ******************

	/*
	In a list of setup parameters for example, the items may be a mix of numbers of different lengths, some being decimal.
	Therfore when the list element is created you cannot specify the no of characters or decimal places.
	The ValRange will have to be supplied at run-time by the object supplying the items.
	*/

	/// <summary>
	/// Formatting information used to interpret data held by I_UI_Wrapper.
	///	format is e_edOneShort by default : Only allow editing at one position. Only show significant digits.
	///		may be modified by any/all of : 
	///		e_editAll : Editing at all positions.
	///		e_showSign : Show sign for positive numbers.
	///		e_fixedWidth : Show spaces for non-significant digits and sign (if range can be negative).
	/// editablePlaces is number of edit positions when placed in edit. 
	///	Unless specified, editablePlaces is calculated from width of maxVal.
	/// _cursorPos is used to hold the cursor-offset during selection and edit.
	/// </summary>
	class ValRange {
	public:
		ValRange(int format, int32_t minVal, int32_t maxVal, int noOfDecPlaces = 0);
		ValRange(int format, int32_t minVal, int32_t maxVal, int noOfDecPlaces, int noOfEditPlaces);
		ValRange(int stringLength = 0);
		bool formatIs(Format tryFormat) const;
		int spaceForDigits(int32_t val) const;

		int32_t minVal = 0; // int32_t to handle dateTime values
		int32_t maxVal = 0;
		unsigned char editablePlaces = 1; // doubles as max string length
		unsigned char noOfDecPlaces = 0; // doubles as current string length
		unsigned char format = 0;
		mutable unsigned char _cursorPos = 0;
	};

	/// <summary>
	/// This class wraps data values, typically obtained from a database.
	/// It is constructed with the value and a ValRange formatting object.
	/// It facilitates streaming and editing by providing a refernce to an I_Field_Interface object via ui().
	/// </summary>
	class I_UI_Wrapper {
	public:
		I_UI_Wrapper() = default;
		I_UI_Wrapper(int32_t val, ValRange valRangeArg)
			: val(val), valRange(valRangeArg) {
		}
		virtual const I_Field_Interface & ui() const { return const_cast<I_UI_Wrapper*>(this)->ui(); }
		virtual I_Field_Interface & ui() = 0;
		I_UI_Wrapper & operator= (int32_t valArg) { val = valArg; return *this; }
		//I_UI_Wrapper & operator= (const I_UI_Wrapper & wrapper) { val = wrapper.val; valRange = wrapper.valRange; return *this; }
		int32_t val;
		ValRange valRange;
	};
}