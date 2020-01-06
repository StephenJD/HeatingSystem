#ifndef __TYPE_TRAITS_H
#define __TYPE_TRAITS_H

//EXAMPLE:

struct __true_type {
};

struct __false_type {
};

template <class _Tp> struct _Is_integer {
  typedef __false_type _Integral;
};

template <>
struct _Is_integer<bool> {
  typedef __true_type _Integral;
};


template <>
struct _Is_integer<char> {
  typedef __true_type _Integral;
};


template <>
struct _Is_integer<signed char> {
  typedef __true_type _Integral;
};


template <>
struct _Is_integer<unsigned char> {
  typedef __true_type _Integral;
};

#ifdef __STL_HAS_WCHAR_T

template <>
struct _Is_integer<wchar_t> {
  typedef __true_type _Integral;
};

 /* __STL_HAS_WCHAR_T */
#endif

template <>
struct _Is_integer<short> {
  typedef __true_type _Integral;
};

template <>
struct _Is_integer<unsigned short> {
  typedef __true_type _Integral;
};

template <>
struct _Is_integer<int> {
  typedef __true_type _Integral;
};

template <>
struct _Is_integer<unsigned int> {
  typedef __true_type _Integral;
};

template <>
struct _Is_integer<long> {
  typedef __true_type _Integral;
};

template <>
struct _Is_integer<unsigned long> {
  typedef __true_type _Integral;
};

#ifdef __STL_LONG_LONG

template <>
struct _Is_integer<long long> {
  typedef __true_type _Integral;
};

template <>
struct _Is_integer<unsigned long long> {
  typedef __true_type _Integral;
};

#endif /* __STL_LONG_LONG */

#endif /* __TYPE_TRAITS_H */

// Local Variables:
// mode:C++
// End:
