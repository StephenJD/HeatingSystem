#pragma once
#include <Arduino.h>

namespace flag_enum {
	template <bool B, typename T, typename F>
	struct conditional { typedef T type; };

	template <typename T, typename F>
	struct conditional<false, T, F> { typedef F type; };

	template <size_t Width>
	struct SmallestType {
		typedef
			typename conditional<Width == 0, void,
			typename conditional<Width <= 1, uint8_t,
			typename conditional<Width <= 2, uint16_t,
			typename conditional<Width <= 4, uint32_t,
			typename conditional<Width <= 8, uint64_t,
			void>::type>::type>::type>::type>::type type;
	};

	/// <summary>
	/// EnumType defines the names of the flags.
	/// Each flag refers to a different bit in the underlying value.
	/// The underlying value is the smallest size having noOfFlags-bits.
	/// The remaining bits, in excess of noOfFlags may be used as ordinary unsigned data.
	/// The FE_Obj contains the flag-enum value.
	/// </summary>
	/// <typeparam name="EnumType"></typeparam>
	template <typename EnumType, int noOfFlags = 8>
	class FE_Obj {
		using Base = typename SmallestType<1 + (noOfFlags-1)/8>::type;
		static constexpr Base TOP_BIT = 1 << ((sizeof(Base) * 8) - 1);
	public:		
		using type = EnumType;
		FE_Obj() = default;
		FE_Obj(Base val) : _base(val) {	static_assert(1+(noOfFlags - 1) / 8 <= sizeof(Base),"Type too small" );}
		FE_Obj(EnumType val) : _base(val) {}
		static void min_max(FE_Obj& min, FE_Obj& max) {
			if (min & ~MAX_VALUE > max & ~MAX_VALUE) {
				Base tempMin = min;
				min = max;
				max = tempMin;
			}
		}
		constexpr operator EnumType() const { return EnumType(_base); }
		//bool get(EnumType pos) const { return _base & (TOP_BIT >> int(pos)); }
		constexpr bool is(EnumType pos) const { return _base & (TOP_BIT >> int(pos)); }
		constexpr bool is(int pos) const { return _base & (TOP_BIT >> pos); }
		constexpr bool is_not(EnumType pos) const { return !is(pos); }
		constexpr bool is_not(int pos) const { return !is(pos); }
		constexpr bool all() const { return flags() == Base(~MAX_VALUE); }
		constexpr bool any() const { return flags() != 0; }
		constexpr bool none() const {	return flags() == 0; }
		constexpr EnumType firstSet() const { uint16_t pos = 0; while (pos < noOfFlags && not(pos)) ++pos; return EnumType(pos); }
		constexpr Base flags() const { return _base & ~MAX_VALUE; }
		//bool get(int pos) const { return get(EnumType(pos)); }
		constexpr Base value() const { return _base & MAX_VALUE; }
		constexpr explicit operator Base() const { return _base; }

		constexpr FE_Obj& setWhole(Base whole) { _base = whole; return *this; }
//#ifdef __AVR__
		EnumType firstNotSet() const { uint16_t pos = 0; while (pos < noOfFlags && is(pos)) ++pos; return EnumType(pos); }
		FE_Obj& set(EnumType pos) { _base |= (TOP_BIT >> int(pos)); return *this; }
		FE_Obj& set(EnumType pos, bool val) { if (val) set(pos); else clear(pos); return *this; }
		FE_Obj& clear(EnumType pos) { _base &= ~(TOP_BIT >> int(pos)); return *this; }
		FE_Obj& setValue(Base val) { _base |= (val & MAX_VALUE); return *this; }
		FE_Obj& clear() { _base = value(); return *this; }
		FE_Obj& setFlags(Base flags) { _base = value() | (flags & ~MAX_VALUE); return *this; }
//#else
//		constexpr FE_Obj& clear() { _base = value(); return *this; }
//		constexpr EnumType firstNotSet() const { uint16_t pos = 0; while (pos < noOfFlags && is(pos)) ++pos; return EnumType(pos); }
//		constexpr FE_Obj& set(EnumType pos) { _base |= (TOP_BIT >> int(pos)); return *this; }
//		constexpr FE_Obj& set(EnumType pos, bool val) { if (val) set(pos); else clear(pos); return *this; }
//		constexpr FE_Obj& clear(EnumType pos) { _base &= ~(TOP_BIT >> int(pos)); return *this; }
//		constexpr FE_Obj& setValue(Base val) { _base |= (val & MAX_VALUE); return *this; }
//		constexpr FE_Obj& setFlags(Base flags) { _base = value() | (flags & ~MAX_VALUE); return *this; }
//#endif
		//void clear(int pos) { clear(EnumType(pos)); }
		constexpr FE_Obj& operator += (EnumType pos) { set(pos); return *this; }
		//FlagEnum& operator += (int pos) { return (*this) += (EnumType(pos)); }
		constexpr FE_Obj& operator -= (EnumType pos) { clear(pos); return *this; }
		//FlagEnum& operator -= (int pos) { return (*this) -= (EnumType(pos)); }
		constexpr FE_Obj& operator = (Base val) { _base = val; return *this; }
		constexpr FE_Obj& operator = (EnumType val) { _base = val; return *this; }
		static constexpr Base MAX_VALUE = (TOP_BIT >> (noOfFlags - 1)) - Base(1);
//#ifdef ZPSIM
//		void printFlags() {
//			for (unsigned int i = 0; i < sizeof(Base) * 8; ++i) {
//				logger() << is(i) << ", ";
//			}
//			logger() << endl;
//		}
//#endif
	private:
		Base _base = 0;
	};

	/// <summary>
	/// EnumType defines the names of the flags.
	/// Each flag refers to a different bit in the underlying value.
	/// The underlying value is the smallest size having noOfFlags-bits.
	/// The remaining bits, in excess of noOfFlags may be used as ordinary unsigned data.
	/// Operations modify the reference value passed into the constructor.
	/// </summary>
	/// <typeparam name="EnumType"></typeparam>
	template <typename EnumType, int noOfFlags = 8>
	class FE_Ref {
		using Base = typename SmallestType<1 + (noOfFlags-1)/8>::type;
		static constexpr Base TOP_BIT = 1 << ((sizeof(Base) * 8) - 1);
	public:		
		using type = EnumType;
		FE_Ref(volatile EnumType& val) : _base_ptr(static_cast<volatile Base*>(&val)) {}
		FE_Ref(volatile Base& val) : _base_ptr(&val) {}
		/*template<typename T>
		FE_Ref(T& val) { 
			static_assert(1 + (noOfFlags - 1) / 8 <= sizeof(T), "Type used for flag-enum is too small"); 
			_base_ptr = static_cast<volatile Base*>(&val);
		}*/
		//FE_Ref(Base& val) : _base_ptr(&val) {}
		operator EnumType() const { return  EnumType(* _base_ptr); }

		bool is(EnumType pos) const { return *_base_ptr & (TOP_BIT >> int(pos)); }
		bool is(int pos) const { return *_base_ptr & (TOP_BIT >> pos); }
		bool is_not(EnumType pos) const { return !is(pos); }
		bool is_not(int pos) const { return !is(pos); }
		Base flags() const { return *_base_ptr & ~MAX_VALUE; }
		Base value() const { return *_base_ptr & MAX_VALUE; }
		explicit operator Base() const { return *_base_ptr; }

		FE_Ref& setWhole(Base whole) { *_base_ptr = whole; return *this; }
		FE_Ref& setFlags(Base flags) { *_base_ptr = value() | (flags & ~MAX_VALUE); return *this; }
		FE_Ref& set(EnumType pos) { *_base_ptr |= (TOP_BIT >> int(pos)); return *this; }
		FE_Ref& set(EnumType pos, bool val) { if (val) set(pos); else clear(pos); return *this; }
		FE_Ref& clear(EnumType pos) { *_base_ptr &= ~(TOP_BIT >> int(pos)); return *this; }
		FE_Ref& operator += (EnumType pos) { set(pos); return *this; }
		FE_Ref& operator -= (EnumType pos) { clear(pos); return *this; }
		FE_Ref& operator = (volatile EnumType& val) = delete;
		FE_Ref& operator = (volatile Base& val) = delete;
		FE_Ref& setValue(Base val) { *_base_ptr |= (val & MAX_VALUE); return *this;	}
		static constexpr Base MAX_VALUE = (TOP_BIT >> (noOfFlags - 1)) - Base(1);
//#ifdef ZPSIM
//		void printFlags() {
//			for (unsigned int i = 0; i < sizeof(Base) * 8; ++i) {
//				logger() << is(i) << ", ";
//			}
//			logger() << endl;
//		}
//#endif
	private:
		volatile Base* _base_ptr;
	};
}