#ifndef RANGE_UTILS_COMMON_H_INC
#define RANGE_UTILS_COMMON_H_INC

#ifdef RANGE_UTILS_USE_RANGEv3
#include <range/v3/range/concepts.hpp>
namespace range_utils {
	namespace ranges = ::ranges;
}
#else
#include <ranges>
namespace range_util {
	namespace ranges = std::ranges;
}
#endif

namespace range_utils {
	template <class T> struct remove_rvalue_ref { typedef T type; };
	template <class T> struct remove_rvalue_ref<T&> { typedef T& type; };
	template <class T> struct remove_rvalue_ref<T&&> { typedef T type; };
}
#endif
