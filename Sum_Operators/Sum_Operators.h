#pragma once

namespace Rel_Ops {
	template <class T, class R> constexpr bool operator!= (const T& x, const R& y) { return !(x == y); }
	template <class T, class R> constexpr bool operator>  (const T& x, const R& y) { return y<x; }
	template <class T, class R> constexpr bool operator<= (const T& x, const R& y) { return !(y<x); }
	template <class T, class R> constexpr bool operator>= (const T& x, const R& y) { return !(x<y); }
}

namespace Sum_Ops {

	// add std::is_copy_constructible<Ex1>::value << '\n' to return using 1 and 0 
	// instead of C(0) and c(1) to support types that take integer arguments rather than copy arguments
	// see https://jguegant.github.io/blogs/tech/sfinae-introduction.html

	struct true_type { bool b; };
	struct false_type { bool b; };

	template <bool B>
	struct typed_bool {
		true_type operator()() { return{ true }; }
	};

	template <>
	struct typed_bool<false> { 
		false_type operator()() { return{ true }; }
	};

	template<typename T>
	struct HasPlusEqOfClass
	{
		// takes type and function signature
		template<typename U, U&(U::*)(const U&)> struct SFINAE {};

		// Tests for specific function name with correct signature
		template<typename U> static char Test(SFINAE<U, &U::operator+=>*);

		template<typename U> static int Test(...);

		static const bool Has = sizeof(Test<T>(0)) == sizeof(char);
	};

	template<typename SumOpType>
	SumOpType & UseAddSelfToSelf(SumOpType & s, true_type)
	{
		return s += SumOpType(1);
	}

	template<typename SumOpType>
	SumOpType & UseAddSelfToSelf(SumOpType & s, false_type)
	{
		return s += 1;
	}

	template<typename SumOpType>
	SumOpType & operator++ (SumOpType & s)
	{
		return UseAddSelfToSelf(s,
			typed_bool<HasPlusEqOfClass<SumOpType>::Has>{}());
	}


	template<class C>
	C operator- (const C & arg) {return C(0) -= arg;}

	//template<class C>
	//C & operator++ (C & arg) { 
	//	// does C.operator+=(C) exist?
	//	return arg += C(1);
	//}

	template<class C>
	C & operator-- (C & arg) {return arg -= C(1);}

	template<class C>
	C operator++ (C & arg, int) {
		C original = arg;
		++arg;
		return original;
	}

	template<class C>
	const C operator-- (C & arg, int) {
		const C original = arg;
		--arg;
		return original;
	}

	// Global binary operators
	template<class C>
	C operator+(const C & lhs, const C & rhs) { C result = lhs; return result += rhs; }

	template<class C>
	C operator-(const C & lhs, const C & rhs) { C result = lhs; return result -= rhs; }

}