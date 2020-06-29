//====================================================================
// File:        BitField.h
// Description: Declaration of UBitField class
//====================================================================
#pragma once
#include <Arduino.h>
#include "..\Sum_Operators\Sum_Operators.h"
//#include <type_traits.h>
//#include <numeric_limits.h>
//#ifdef max
//#undef max
//#undef min
//#endif
//
//#ifdef typename
//#undef typename
//#endif

namespace BitFields {
	// Provides compact and predictable Bitfield.
	// The BitFields are intended to be used in a union.
	// Unions do NOT incur extra padding.
	// However, nested Struct or Bitfield is padded to the word width.
	using namespace Sum_Ops;

	template <bool B, typename T, typename F>
	struct conditional { typedef T type; };

	template <class T, class F>
	struct conditional<false, T, F> { typedef F type; };

	template <size_t Width>
	struct SmallestType {
		typedef
			typename conditional<Width == 0, void,
			typename conditional<Width <= 8, uint8_t,
			typename conditional<Width <= 16, uint16_t,
			typename conditional<Width <= 32, uint32_t,
			typename conditional<Width <= 64, uint64_t,
			void>::type>::type>::type>::type>::type type;
	};

	/// <summary>
	/// Creates char-sized or larger bitfield collections that are portable
	/// </summary>
	template <typename IntType, int start, int bits = 1>
	class UBitField {
		static_assert(start >= 0, "start must be >= 0");
		static_assert(bits > 0, "bits must be > 0");
		//static_assert((1LL << (start + bits)) - 1 <= std::numeric_limits<IntType>::max(), "Integer type too small");
		//static_assert(is_integral< IntType >::value && is_unsigned< IntType >::value, "IntType must be an unsigned integer type");

	public:
		/*constexpr*/ UBitField() = default;
		constexpr UBitField(IntType val) : base((base & ~(mask << start)) | ((val & mask) << start)) {}
		constexpr UBitField(IntType val, bool zeroed) : base(IntType{0} | ((val & mask) << start)) {}

		// Queries
		typedef typename SmallestType<bits>::type RT;
		constexpr operator RT() const {
			return (base >> start) & mask;
		}

		constexpr IntType asBase() const {
			return base;
		}

		// Modifiers
		UBitField & operator=(IntType val) {
			base = (base & ~(mask << start)) | ((val & mask) << start);
			return *this;
		}

		UBitField & operator+=(IntType val) {
			return operator=(operator RT() + val);
		}

		UBitField & operator-=(IntType val) {
			return operator=(operator RT() - val);
		}

	protected:
		static constexpr IntType mask = (IntType(1) << bits) - IntType(1);
		//enum :IntType { mask = (1u << bits) - 1u };
		IntType base; // must not initialise because only one member of a union is allowed to be initialised
	};



}
