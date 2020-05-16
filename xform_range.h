#ifndef XFORM_RANGE_H_INC
#define XFORM_RANGE_H_INC

#include <iterator>
#include <type_traits>
#include <concepts>
#include "range_utils_common.h"

namespace range_utils {

#ifndef _MSC_VER
    template <ranges::input_range R, std::invocable<typename ranges::range_reference_t<R>> Fp>
    requires ranges::viewable_range<R>
#else
    //disable concept check for this on MSVC because the compiler chokes
    template <class R, class Fp>
#endif
    class xform_range_t {
        R range;
        Fp fp;
    public:
        class iterator_t {
            friend class xform_range_t<R, Fp>;
            using wrapped_t = typename ranges::iterator_t<R>;
            using wrapped_ref_t = std::iter_reference_t<wrapped_t>;
            using wrapped_value_t = std::iter_value_t<wrapped_t>;

            static constexpr bool ordered = std::totally_ordered<wrapped_t>;
            static constexpr bool bidirectional = ranges::bidirectional_range<R>;
            static constexpr bool random_access = ranges::random_access_range<R>;

            wrapped_t iter;
            Fp functor;

            //only friends (parent) can instantiate
            explicit iterator_t(const wrapped_t& i, const Fp& f)
                : iter{ i }, functor(f) {}
            explicit iterator_t(wrapped_t&& i, Fp&& f)
                : iter{ i }, functor(f) {}
        public:
            iterator_t() = default;
            ~iterator_t() = default;
            iterator_t(const iterator_t&) = default;
            iterator_t(iterator_t&&) = default;
            iterator_t& operator=(const iterator_t&) = default;
            iterator_t& operator=(iterator_t&&) = default;

            using value_type = decltype(functor(*iter));
            using reference_type = value_type;
            using difference_type = std::ptrdiff_t;

            iterator_t& operator++() {
                ++iter;
                return *this;
            }
            iterator_t operator++(int) {
                iterator_t ret{ *this };
                ++(*this);
                return ret;
            }
            // Only provide decrementing if it's supported
            template <class U = iterator_t>
            typename std::enable_if<U::bidirectional,
                iterator_t&>::type operator--() {
                --iter;
                return *this;
            }
            template <class U = iterator_t>
            typename std::enable_if<U::bidirectional,
                iterator_t&>::type operator--(int) {
                iterator_t ret{ *this };
                --(*this);
                return ret;
            }

            reference_type operator*() const {
                return functor(*iter);
            }
            reference_type operator->() const {
                return functor(*iter);
            }

            bool operator==(const iterator_t& other) const {
                return iter == other.iter;
            }
            bool operator!=(const iterator_t& other) const {
                return iter != other.iter;
            }

            // Only provide </<=/>/>= if they're supported
            template <class U = iterator_t>
            typename std::enable_if<U::ordered,
                bool>::type operator<(const iterator_t& other) const {
                return iter < other.iter;
            }
            template <class U = iterator_t>
            typename std::enable_if<U::ordered,
                bool>::type operator>=(const iterator_t& other) const {
                return iter >= other.iter;
            }
            template <class U = iterator_t>
            typename std::enable_if<U::ordered,
                bool>::type operator>(const iterator_t& other) const {
                return iter > other.iter;
            }
            template <class U = iterator_t>
            typename std::enable_if<U::ordered,
                bool>::type operator<=(const iterator_t& other) const {
                return iter <= other.iter;
            }

            // Only provide addition/subtraction if they're supported
            template <class T, class U = iterator_t>
            typename std::enable_if<U::random_access,
                iterator_t&>::type operator+=(const T& rhs) {
                iter += rhs;
                return *this;
            }
            template <class T, class U = iterator_t>
            typename std::enable_if<U::random_access,
                iterator_t&>::type operator-=(const T& rhs) {
                iter -= rhs;
                return *this;
            }
            template <class T, class U = iterator_t>
            typename std::enable_if<U::random_access,
                iterator_t>::type operator+(const T& rhs) const {
                iterator_t ret{ *this };
                ret += rhs;
                return ret;
            }
            template <class T, class U = iterator_t>
            typename std::enable_if<U::random_access,
                iterator_t>::type operator-(const T& rhs) const {
                iterator_t ret{ *this };
                ret -= rhs;
                return ret;
            }

        };

        xform_range_t(R r, Fp f = Fp{}) : range{ r }, fp{ f } {}
        iterator_t begin() const {
            return iterator_t{ std::begin(range), fp };
        }
        iterator_t end() const {
            return iterator_t{ std::end(range), fp };
        }
    };

    template <class R, class F>
    auto xform(R&& r, F f = F{}) {
        return xform_range_t<typename remove_rvalue_ref<R&&>::type, F>
        (std::forward<R>(r), f);
    }

}
#ifdef RANGE_UTILS_USE_RANGEv3
template <class T, class U>
constexpr bool ranges::enable_view<range_utils::xform_range_t<T, U>> = true;
#else
template <class... Args>
constexpr bool std::ranges::enable_borrowed_range<range_utils::xform_range_t<Args...>> = true;
#endif
#endif
