#pragma once

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
	/// If the FE object is constructed from a modifyable value, it operates on the original.
	/// If the FE object is constructed from a temp/const value, it operates on a copy.
	/// </summary>
	/// <typeparam name="EnumType"></typeparam>
	template <typename EnumType, int noOfFlags = 8>
	class FE {
		using Base = typename SmallestType<1 + noOfFlags/8>::type;
		static constexpr Base TOP_BIT = 1 << ((sizeof(Base) * 8) - 1);
	public:		
		using type = EnumType;
		FE(EnumType& val) : _base_ptr(static_cast<Base*>(&val)) {}
		FE(Base& val) : _base_ptr(&val) {}
		FE(const Base& val) : _base(val), _base_ptr(&_base) {}
		//bool get(EnumType pos) const { return *_base_ptr & (TOP_BIT >> int(pos)); }
		bool is(EnumType pos) const { return *_base_ptr & (TOP_BIT >> int(pos)); }
		bool is(int pos) const { return *_base_ptr & (TOP_BIT >> pos); }
		Base flags() const { return *_base_ptr & ~MAX_VALUE; }
		//bool get(int pos) const { return get(EnumType(pos)); }
		Base value() const { return *_base_ptr & MAX_VALUE; }
		explicit operator Base() const { return *_base_ptr; }

		FE& setWhole(Base whole) { *_base_ptr = whole; return *this; }
		FE& setFlags(Base flags) { *_base_ptr = value() | (flags & ~MAX_VALUE); return *this; }
		FE& set(EnumType pos) { *_base_ptr |= (TOP_BIT >> int(pos)); return *this; }
		//void set(int pos) { set(EnumType(pos)); }
		FE& set(EnumType pos, bool val) { if (val) set(pos); else clear(pos); return *this; }
		//void set(int pos, bool val) { set(EnumType(pos), val); }
		FE& clear(EnumType pos) { *_base_ptr &= ~(TOP_BIT >> int(pos)); return *this; }
		//void clear(int pos) { clear(EnumType(pos)); }
		FE& operator += (EnumType pos) { set(pos); return *this; }
		//FlagEnum& operator += (int pos) { return (*this) += (EnumType(pos)); }
		FE& operator -= (EnumType pos) { clear(pos); return *this; }
		//FlagEnum& operator -= (int pos) { return (*this) -= (EnumType(pos)); }
		FE& operator = (Base val) { setValue(val); return *this; }
		void setValue(Base val) { *_base_ptr |= (val & MAX_VALUE); }
		static constexpr Base MAX_VALUE = (TOP_BIT >> (noOfFlags - 1)) - Base(1);
#ifdef ZPSIM
		void printFlags() {
			for (unsigned int i = 0; i < sizeof(Base) * 8; ++i) {
				logger() << is(i) << ", ";
			}
			logger() << endl;
		}
#endif
	private:
		Base* _base_ptr;
		Base _base;
	};
}