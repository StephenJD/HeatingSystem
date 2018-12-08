//#include "Calendar.h"
//#include "Clock.h"
//#include "Data_Spell.h"
//
//namespace Client_DataStructures {
//	using namespace Date_Time;
//	using namespace HardwareInterfaces;
//	using namespace LCD_UI;
//
//	Calendar::Calendar(Dataset_Spell * spells)
//		:_spells(spells)
//	{}
//
//	void Calendar::loadSpellSequence() {
//		auto spellCount = _spells->count();
//		auto firstSpell = getVal(E_firstDT);
//		auto arrSize = spellCount * 2;
//		delete _sequence;
//		_sequence = new startTime[arrSize];
//
//		populateSequence(firstSpell, spellCount, arrSize); // with start and end times
//		sortSequence(arrSize);
//
//		//#if defined DEBUG_INFO
//		//	cout << "Sorted Zone:" << int(epD().index()) << '\n';
//		//	for (U1_ind t = 0; t < arrSize; ++t) {
//		//		cout << (int)t << ": " << DateTime_Stream::getTimeDateStr(_sequence[t].start) << " : " << (int)_sequence[t].spellID << '\n';
//		//	}
//		//#endif
//		removeNestedSpells(arrSize);
//		removeOldSpells(arrSize);
//		// reduce to final sequence
//		sortSequence(arrSize);
//		//#if defined DEBUG_INFO
//		//	cout << "loadSpellSequence for Zone:" << int(epD().index()) << '\n';
//		//	for (U1_ind t = 0; t < arrSize; ++t) {
//		//		cout << (int)t << ": " << DateTime_Stream::getTimeDateStr(_sequence[t].start) << " Yr:" << (int)_sequence[t].start.getYr() << " : " << (int)_sequence[t].spellID << '\n';
//		//	}
//		//#endif
//	}
//
//	void Calendar::populateSequence(int firstSpell, int spellCount, int arrSize) {
//		auto e = spellCount;
//		clock_().refresh();
//		auto dtNow = DateTime{ clock_() };
//		for (int t = 0; t < spellCount; ++t) {
//			auto spell_ID = firstSpell + t;
//			auto thisSpell = f->dateTimesS(spell_ID).dpIndex();
//			_sequence[t].start = f->dateTimesS(spell_ID).getDateTime(spell_ID);
//			if (_sequence[t].start == DateTime(0L)) { // move spares to end
//				_sequence[t].start = JUDGEMEMT_DAY;
//				_sequence[t].spellID = SPARE_SPELL;
//			}
//			else if (f->dailyProgS(thisSpell).getDPtype() >= E_dpDayOff) { // make special DT's -ve and create +ve paired DT at end of period
//				S2_byte period = f->dailyProgS(thisSpell).getVal(E_period);
//				DateTime expiryDate = f->dateTimesS(spell_ID).addDateTime(period);
//				if (expiryDate <= dtNow) { // recycle expired special DT's
//					f->dateTimesS(spell_ID).recycleSpell();
//					_sequence[t].start = JUDGEMEMT_DAY;
//					_sequence[t].spellID = SPARE_SPELL;
//				}
//				else {
//					_sequence[t].spellID = -spell_ID - 1;
//					_sequence[e].start = expiryDate;
//					_sequence[e].spellID = spell_ID;
//					++e;
//				}
//			}
//			else {
//				_sequence[t].spellID = spell_ID;
//			}
//		}
//		for (; e < arrSize; ++e) {
//			_sequence[e].start = JUDGEMEMT_DAY;
//			_sequence[e].spellID = SPARE_SPELL;
//		}
//	}
//
//	void Calendar::removeNestedSpells(int arrSize) {
//		// Set starts for all weekly and end entries between a DP and the next matching -DP to Judgement day to remove them from the sequence
//		int prevWkly;
//		nestedSpecials nestSpec;
//		auto t = 0;
//		while (t < arrSize) {
//			//cout << "t:" << (int)t << "\n";
//			auto thisSpell = _sequence[t].spellID;
//			if (thisSpell == SPARE_SPELL) return;
//			if (thisSpell >= 0) { // weekly or end of special
//				auto nextSpell = nestSpec.addSpell(thisSpell, prevWkly);
//				if (nextSpell == -1) { // prev DT ends after this new weekly or special end
//					_sequence[t].start = JUDGEMEMT_DAY;
//					_sequence[t].spellID = SPARE_SPELL;
//				}
//				else { // current special is ended
//					if (nextSpell == _sequence[t + 1].spellID && _sequence[t].start == _sequence[t + 1].start) { // two DTs ending at the same time
//						_sequence[t].start = JUDGEMEMT_DAY;
//						_sequence[t].spellID = SPARE_SPELL;
//					}
//					else {
//						_sequence[t].spellID = nextSpell;
//						recycleSpell(prevWkly, t, nestSpec);
//					}
//				}
//			}
//			else { // new Special start
//				nestSpec.addSpell(thisSpell, prevWkly);
//				_sequence[t].spellID = -thisSpell - 1; // reset start DP to non-negated value
//			}
//			++t;
//		}
//	}
//
//	void Calendar::recycleSpell(int prevWkly, int t, nestedSpecials & nestSpec) {
//		if (prevWkly != SPARE_SPELL) { // recycle weekly DT?
//			clock_().refresh();
//			auto dtNow = DateTime{ clock_() };
//
//			auto thisSpell = _sequence[t].spellID;
//			if (_sequence[t].start <= dtNow) {
//				f->dateTimesS(prevWkly).recycleSpell();
//			}
//			else if (f->dateTimesS(prevWkly).dpIndex() == f->dateTimesS(thisSpell).dpIndex()) { // adjacent duplicate weekly DPs
//				f->dateTimesS(thisSpell).recycleSpell();
//				nestSpec.addSpell(prevWkly, prevWkly);
//				_sequence[t].start = JUDGEMEMT_DAY;
//				_sequence[t].spellID = SPARE_SPELL;
//			}
//		}
//	}
//
//	void Calendar::removeOldSpells(int arrSize) {
//		clock_().refresh();
//		auto dtNow = DateTime{ clock_() };
//		//startTime debug = _sequence[t+1];
//		for (int t = 0; t < arrSize - 1; ++t) {
//			if (_sequence[t].start != JUDGEMEMT_DAY && _sequence[t + 1].start <= dtNow) {
//				_sequence[t].start = JUDGEMEMT_DAY;
//				_sequence[t].spellID = SPARE_SPELL;
//			}
//			//debug = _sequence[t+1];
//		}
//	}
//
//	Calendar::nestedSpecials::nestedSpecials() :end(0) { spellArr[0] = SPARE_SPELL; };
//
//	int Calendar::nestedSpecials::addSpell(int spellID, int & prevWkly) {
//		// the DP array will contain only one +ve weekly and 0 or more -ve special starts.
//		auto nextSpell = -1; prevWkly = SPARE_SPELL;
//		int i = end - 1;
//		if (spellID >= 0) { // if +ve it is either a replacement weekly DP or the end of a -ve special.
//			if (spellArr[i] == -spellID - 1) { // prev DT is start of this special
//				--end;
//				nextSpell = spellArr[end - 1] >= 0 ? spellArr[end - 1] : -spellArr[end - 1] - 1; // Prev DT
//			}
//			else { // search backwards for a -ve entry ...
//				while (i >= 0 && spellArr[i] != -spellID - 1) {
//					--i;
//				}
//				if (i >= 0) { // found the special start, so remove it.
//							  //cout << "Remove:" << int(spellID) << '\n';
//					for (; i < end - 1; ++i) {
//						spellArr[i] = spellArr[i + 1];
//					}
//					--end;
//				}
//				else { // this is a replacement weekly DP
//					   //cout << "Replace Weekly with:" << int(spellID) << '\n';
//					prevWkly = spellArr[0];  // recycle old weekly?
//					spellArr[0] = spellID;
//					nextSpell = spellID >= 0 ? spellID : -spellID - 1;
//					if (end == 0) ++end;
//					else if (spellArr[end - 1] < 0) nextSpell = -1; // new weekly, but prev special not expired
//				}
//			}
//		}
//		else { // this is a new special start
//			   //cout << "New Spec:" << int(spellID) << '\n';
//			spellArr[end] = spellID;
//			++end;
//		}
//		return nextSpell;
//	}
//
//	void Calendar::sortSequence(int count) {
//		// bubble sort startTimes in increasing order
//		for (int i = 1; i < count; ++i) {
//			for (startTime * j = _sequence; j < _sequence + count - i; ++j) {
//				if (*(j + 1) < *j) {
//					startTime temp = *j;
//					*j = *(j + 1);
//					*(j + 1) = temp;
//				}
//			}
//		}
//	}
//
//	int Calendar::getNthSpell(ZdtData & zd, int position) {
//		if (!_sequence) loadSpellSequence();
//		// If position = -2 returns viewedDTposition. If position = 127 returns noOf. 
//		if (position == VIEWED_DT) position = zd.viewedDTposition;
//		int t = 0;
//		while (t < position && _sequence[t].spellID != SPARE_SPELL) ++t;
//		if (position == 127) return t;
//		else {
//			zd.expiryDate = _sequence[t + 1].start;
//			zd.nextSpell = _sequence[t + 1].spellID;
//			return _sequence[t].spellID;
//		}
//	}
//
//	int Calendar::findSpell(DateTime originalFromDate, DateTime originalToDate) const {
//		int t = 0;
//		while (_sequence[t].spellID != SPARE_SPELL && _sequence[t].start != originalFromDate) ++t;
//		if (_sequence[t].start == originalFromDate && _sequence[t + 1].start == originalToDate)
//			return _sequence[t].spellID;
//		else return -1;
//	}
//
//	int Calendar::currentWeeklySpell(ZdtData & zd, DateTime thisDate) const {
//		auto nextSpell = 255;
//		DateTime nextDateTime;
//		for (int d = zd.firstSpell; d < zd.lastDT; d++) { // find weekly DT with latest start date upto thisDate
//			DateTime tryDateTime = f->dateTimesS(d).getDateTime(d);
//			if (tryDateTime <= thisDate && f->dateTimesS(d).getDPtype() == E_dpWeekly) {
//				if (tryDateTime > nextDateTime) {
//					nextDateTime = tryDateTime;
//					nextSpell = d;
//				}
//			}
//		}
//		return nextSpell;
//	}
//}
