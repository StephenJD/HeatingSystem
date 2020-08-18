#pragma once
#include <Arduino.h>
#include "UI_Collection.h"
#include "..\..\..\Conversions\Conversions.h" // for client use of formatting enum

namespace LCD_UI {
	using namespace GP_LIB; // for client use of formatting enum

	class I_Streaming_Tool;

	// ************** valrange ******************

	/*
	In a list of setup parameters for example, the items may be a mix of numbers of different lengths, some being decimal.
	Therfore when the list element is created you cannot specify the no of characters or decimal places.
	The ValRange will have to be supplied at run-time by the object supplying the items.
	*/

	/// <summary>
	/// Formatting information used to interpret data held by I_Data_Formatter.
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
	/// This abstract class wraps data values with ValRange formatting information.
	/// Derived classes direct streaming and editing to an I_Streaming_Tool object via ui().
	/// </summary>
	class I_Data_Formatter {
	public:
		I_Data_Formatter() = default;
		I_Data_Formatter(int32_t initialVal, ValRange valRangeArg)
			: val(initialVal), valRange(valRangeArg) {
		}
		virtual const I_Streaming_Tool & ui() const { return const_cast<I_Data_Formatter*>(this)->ui(); }
		virtual I_Streaming_Tool & ui() = 0;
		I_Data_Formatter & operator= (int32_t valArg) { val = valArg; return *this; }
		int32_t val;
		ValRange valRange;
	};
}