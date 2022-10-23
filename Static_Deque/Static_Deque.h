#pragma once
#include <Arduino.h>

template <int CAPACITY, typename intType = int16_t >
class Deque {
public:
	Deque() : _queue{ 0 } {}
	using ValType = intType;
	// Queries
	intType first() const { return peek(0); }
	intType last(int moveBy = 0) const;
	intType peek(int pos) const;
	uint8_t size() const { return _noOfValues; }
	uint8_t capacity() const { return CAPACITY; }
	// Modifiers
	void push(intType val);
	intType& peek(int pos);
	intType popFirst();
	intType popLast();
	void prime(intType val);
	void prime_n(intType val, int count);
	void clear();
	// Iteration
	class Iterator {
	public:
		using DequeT = Deque<CAPACITY, intType>;
		Iterator(DequeT* deque) : _deque(deque) {}
		Iterator(DequeT* deque, uint8_t pos) : _deque(deque), _pos(pos) {}
		typename DequeT::ValType& operator*() const { return _deque->peek(_pos); }
		typename DequeT::ValType* operator->() { return &_deque->peek(_pos); }

		Iterator& operator++() { ++_pos; return *this; }
		Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

		friend bool operator== (const typename DequeT::Iterator& a, const typename DequeT::Iterator& b) { return a._pos == b._pos; };
		friend bool operator!= (const typename DequeT::Iterator& a, const typename DequeT::Iterator& b) { return !(a==b); };
	private:
		DequeT* _deque;
		uint8_t _pos = 0;
	};
	Iterator begin() { return Iterator(this); }
	Iterator end() { return Iterator(this, size()); }
protected:
	// Queries
	uint8_t nextPos(int moveBy = 1) const;
	uint8_t currPos() const { return nextPos(0); }
	intType _queue[CAPACITY];
	uint8_t _endPos = 0;
	uint8_t _noOfValues = 0;
};

inline int trueModulo(int num, int divisor) {	return ((num % divisor) + divisor) % divisor; }
// Queries
template <int CAPACITY, typename intType>
uint8_t Deque<CAPACITY, intType>::nextPos(int moveBy) const {
	if (_noOfValues == 0) return 0;
	auto startPos = trueModulo(_endPos - _noOfValues, CAPACITY);
	auto newRelativePos = trueModulo(_noOfValues - 1 + moveBy, _noOfValues);
	return uint8_t(trueModulo(startPos + newRelativePos, CAPACITY));
}

template <int C, typename intType>
intType Deque<C, intType>::peek(int pos) const {
	return _queue[nextPos(pos+1)];
}

template <int C, typename intType>
intType& Deque<C, intType>::peek(int pos) {
	return _queue[nextPos(pos+1)];
}

template <int C, typename intType>
intType Deque<C, intType>::last(int moveBy) const {
	return _noOfValues ? _queue[nextPos(moveBy)] : 0;
}

// Modifiers

template <int CAPACITY, typename intType>
void Deque<CAPACITY, intType>::push(intType val) {
	_queue[_endPos] = val;
	if (_noOfValues < CAPACITY) ++_noOfValues;
	_endPos = trueModulo(++_endPos, CAPACITY);
}


template <int C, typename intType>
void Deque<C, intType>::clear() {
	for (auto& item : _queue) item = intType(0);
}

template <int C, typename intType>
void Deque<C, intType>::prime(intType val) {
	for (auto& item : _queue) item = val;
}

template <int C, typename intType>
void Deque<C, intType>::prime_n(intType val, int count) {
	for (auto& item : _queue) {
		item = val;
		--count;
		if (count == 0) break;
	}
}

template <int C, typename intType>
intType Deque<C, intType>::popFirst() {
	auto firstItem = first();
	if (_noOfValues) --_noOfValues;
	return firstItem;
}

template <int CAPACITY, typename intType>
intType Deque<CAPACITY, intType>::popLast() {
	auto lastItem = last();
	if (_noOfValues) --_noOfValues;
	_endPos = trueModulo(_endPos-1, CAPACITY);
	return lastItem;
}

// Global Helper Functions
template <int C, typename intType>
float getSum(Deque<C,intType> queue) {
	float sum = 0;
	auto noOfVals = queue.size();
	for (auto& item : queue) sum += item;
	return sum;
}

template <int C, typename intType>
float getAverage(Deque<C, intType> queue) {
	return queue.size() ? getSum(queue) / float(queue.size()) : 0;
}

template <int CAPACITY, typename intType = int16_t >
class Fat_Deque : public Deque<CAPACITY, intType> {
public:
	bool hasChangedDirection() const;
	intType maxVal() const {return _max;}
	intType minVal() const { return _min; }
	// Modifier
	void push(intType val);

private:
	intType _max = 0;
	intType _min = 0;
};

template <int C, typename intType>
bool Fat_Deque<C, intType>::hasChangedDirection() const {
	auto last = Deque<C, intType>::last();
	auto prev = Deque<C, intType>::last(-1);
	if (Deque<C, intType>::size() > 2) {
		if (prev == _max || prev == _min) {
			if (last != prev) return true;
		}
	}
	return false;
}

template <int C, typename intType>
void Fat_Deque<C, intType>::push(intType val) {
	Deque<C, intType>::push(val);
	if (Deque<C, intType>::size() == 1) {
		_max = val;
		_min = val;
	} else {
		if (val > _max) _max = val;
		if (val < _min) _min = val;
	}
}