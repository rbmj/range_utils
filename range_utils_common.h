#ifndef RANGE_UTILS_COMMON_H_INC
#define RANGE_UTILS_COMMON_H_INC

template <class T> struct remove_rvalue_ref { typedef T type; };
template <class T> struct remove_rvalue_ref<T&> { typedef T& type; };
template <class T> struct remove_rvalue_ref<T&&> { typedef T type; };

#endif
