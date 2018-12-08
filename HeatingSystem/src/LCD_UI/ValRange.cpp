#include "ValRange.h"
#include "Convertions.h"

namespace LCD_UI {
	using namespace GP_LIB;
	// *************** ValRange : Class that maintains state of edited value  ******************

	ValRange::ValRange(int format, int32_t minVal, int32_t maxVal, int noOfDecPlaces)
		: editablePlaces(significantDigits(maxVal)),
		minVal(minVal),
		maxVal(maxVal),
		noOfDecPlaces(noOfDecPlaces),
		_cursorPos(editablePlaces),
		format(format)
	{}

	ValRange::ValRange(int stringLength)
		: editablePlaces(stringLength)
		,_cursorPos(0)
	{}

	ValRange::ValRange(int format, int32_t minVal, int32_t maxVal, int noOfDecPlaces, int noOfEditPlaces)
		: editablePlaces(noOfEditPlaces),
		minVal(minVal),
		maxVal(maxVal),
		noOfDecPlaces(noOfDecPlaces),
		_cursorPos(editablePlaces),
		format(format)
	{}

	bool ValRange::formatIs(Format tryFormat) const {
		if (tryFormat == e_edOneShort) {
			return (format & e_editAll) == 0;
		} else return (format & tryFormat) != 0; 
	};

	int ValRange::spaceForDigits(int32_t val) const {
		int spaceForDigits;
		if (formatIs(e_fixedWidth)) {
			spaceForDigits = editablePlaces;
		}
		else {
			spaceForDigits = significantDigits(val);
			if (spaceForDigits <= noOfDecPlaces) ++spaceForDigits; // for leading zero
		}
		return spaceForDigits;
	};

	//int ValRange::getDisplayedWidth(int val) {
	//	int digitPlaces = significantDigits(val);
	//	int nonSigPlaces = 0;
	//	if (noOfDecPlaces > 0) {
	//		if (digitPlaces <= noOfDecPlaces) {
	//			++digitPlaces; // add leading zero
	//		}
	//		++nonSigPlaces; // add dec point
	//	}
	//	if (editablePlaces > digitPlaces) digitPlaces = editablePlaces;
	//	if (val < 0 || editablePlaces < 0) { ++nonSigPlaces; } // add sign
	//	digitPlaces += nonSigPlaces;
	//	_cursorPos = digitPlaces;
	//	return digitPlaces;
	//}
}