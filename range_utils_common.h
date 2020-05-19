#ifndef RANGE_UTILS_COMMON_H_INC
#define RANGE_UTILS_COMMON_H_INC

#ifdef RANGE_UTILS_USE_RANGEv3
#include <range/v3/range/concepts.hpp>
namespace concepts {
    template <class F, class... Args>
    concept invocable = requires (F&& f, Args&&... args) {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    };
}
namespace range_utils {
    namespace ranges = ::ranges;
    namespace concepts = ::concepts;
}
#else
#include <ranges>
namespace range_utils {
    namespace ranges = std::ranges;
    namespace concepts = std;
}
#endif

namespace range_utils {
    template <class T> struct remove_rvalue_ref { typedef T type; };
    template <class T> struct remove_rvalue_ref<T&> { typedef T& type; };
    template <class T> struct remove_rvalue_ref<T&&> { typedef T type; };
}
#endif
