#include <Date_Time.h>
#include <Conversions.h>

namespace Date_Time {
	using namespace GP_LIB;
	using namespace Rel_Ops;
	using namespace Sum_Ops;

	DateTime & DateTime::addOffset(incDTfld incF) {
		switch (incF.field) {
		case dd: addDays(incF.offset); break;
		case mm: setMonth(month()+incF.offset); addDays(0); break;
		case yy: setYear(year() +incF.offset);  addDays(0); break;
		case hh: addDays(addClockTime(incF.offset*6)); break;
		case m10: addDays(addClockTime(incF.offset)); break;
		}
		return *this;
	}

	DateTime & DateTime::addOffset_NoRollover(incDTfld incF, DateTime now) {
		// If changing day/month/hour/min back before now, first show now, then if going back again show original date-time next year.
		static DateTime savedDateTime(now);
		bool isShowingNow = (*this == now);
		if (!isShowingNow) savedDateTime = *this;

		if (isShowingNow && incF.offset < 0) *this = savedDateTime; // restore dateTime and then apply offset
		if (incF.field == dd) {
			addDays(incF.offset); // incrementing days overflow to the month/year.
		}
		else if (incF.field == mm) setMonth(nextIndex(1, month(), 12, incF.offset)); // incrementing mm, hh, mins do not overflow
		else if (incF.field == yy) setYear(nextIndex(0, year(), 127, incF.offset)); // incrementing mm, hh, mins do not overflow
		else { // hh or m10
			if (incF.field == hh) {
				setHrs(nextIndex(0, hrs(), 23, incF.offset));
			} else {
				setMins10(nextIndex(0, mins10(), 5, incF.offset));
			}
			savedDateTime.addOffset(incF);
		}

		if (*this < now) { 
			if (!isShowingNow && incF.offset < 0) 
				*this = now; // first attempt at going back before now, so stop at now.
			else {
				//if (incF.field <= dd)
					addOffset({ yy,1 }); // we have persisted in going back before now.
				//else if (incF.field == hh) addOffset({dd,1});
				//else addOffset({ hh,1 });
			}
		}
		else if (now)
		{
			auto tryPrevYear = DateTime(*this).addOffset({ yy,-1 });
			if (tryPrevYear >= now)
				*this = tryPrevYear;
		}		
		return *this;
	}

	bool DateTime::isValid() const {
		if (mins10() > 5) return false;
		if (hrs() > 23) return false;
		if (day() > 31) return false;
		if (month() > 12) return false;
		if (year() > 127) return false;
		return true;
	};

	int32_t DateTime::minsTo(const DateTime & minsTill) const {
		auto dayMins = 0;
		if (date() != minsTill.date()) {
			dayMins = (minsTill.dayOfYear() - dayOfYear()) * 24 * 6;
		}
		return (dayMins - time().asInt() + minsTill.time().asInt() ) * 10;
	}

	DateTime::operator CStr_20 () const {
		CStr_20 timedateStr;
		strcpy(timedateStr, intToString(day(), 2));
		strcat(timedateStr, "/");
		strcat(timedateStr, getMonthStr());
		strcat(timedateStr, "/");
		strcat(timedateStr, intToString(year(), 2));
		strcat(timedateStr, " ");
		strcat(timedateStr, intToString(displayHrs(), 2));
		strcat(timedateStr, ":");
		strcat(timedateStr, intToString(mins10()));
		strcat(timedateStr, "0");
		strcat(timedateStr, (isPM() ? "pm" : "am"));
		return timedateStr;
	}
}