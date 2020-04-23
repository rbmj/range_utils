#ifndef ZIP_RANGE_H_INC
#define ZIP_RANGE_H_INC

#include <ranges>
#include <tuple>
#include <iterator>
#include <concepts>
#include <type_traits>
#include <functional>
#include <cstddef>
#include "range_utils_common.h"

template <std::ranges::viewable_range... R>
class zip_range {
    std::tuple<R...> ranges;
    //these next few functions are fairly ugly, but they're
    //just accumulating each of the begin()s/end()s in a tuple
    template <class T, unsigned N, class First, class... Rest>
    auto begin_impl(T&& t) const {
        auto x = std::tuple_cat(std::move(t),
                std::tuple(std::begin(std::get<N>(ranges))));
        return begin_impl<decltype(x), N+1, Rest...>(std::move(x));
    }
    template <class T, unsigned N>
    auto begin_impl(T&& t) const {
        return std::move(t);
    }
    template <class T, unsigned N, class First, class... Rest>
    auto end_impl(T&& t) const {
        auto x = std::tuple_cat(std::move(t),
                std::tuple(std::end(std::get<N>(ranges))));
        return end_impl<decltype(x), N+1, Rest...>(std::move(x));
    }
    template <class T, unsigned N>
    auto end_impl(T&& t) const {
        return std::move(t);
    }
public:
    using zip_range_t = zip_range<R...>;
    class iterator_t {
        friend class zip_range<R...>;
        using iterator_tuple = std::tuple<std::ranges::iterator_t<R>...>;
        iterator_tuple iterators;

        //convenience types for the Nth subiterator
        template <unsigned N>
        using subiter_t = decltype(std::get<N>(iterators));
        template <unsigned N>
        using subiter_ref_t = std::iter_reference_t<subiter_t<N>>;
        template <unsigned N>
        using subiter_value_t = std::iter_value_t<subiter_t<N>>;

        //the number of subiterators
        static constexpr unsigned num_elements =
            std::tuple_size<iterator_tuple>::value;
        //do all subiterators support </<=/>/>=?
        static constexpr bool ordered =
            std::conjunction<std::integral_constant<bool,
            std::totally_ordered<R>>...>::value;
        //are all subiterators bidirectional?
        static constexpr bool bidirectional =
            std::conjunction<std::integral_constant<bool,
            std::ranges::bidirectional_range<R>>...>::value;
        //are all subiterators random access?
        static constexpr bool random_access =
            std::conjunction<std::integral_constant<bool,
            std::ranges::random_access_range<R>>...>::value;

        //only friends (parent) can instantiate
        explicit iterator_t(const iterator_tuple& r)
            : iterators{r} {}
        explicit iterator_t(iterator_tuple&& r)
            : iterators{r} {}

        //get the Nth subiterator
        template <unsigned N>
        inline auto& get() {
            return std::get<N>(iterators);
        }
        template <unsigned N>
        inline const auto& get() const {
            return std::get<N>(iterators);
        }

        //increment/decrement all subiterators
        template <unsigned N = num_elements - 1>
        inline void increment() {
            ++get<N>();
            if constexpr (N != 0) {
                return increment<N-1>();
            }
        }
        template <unsigned N = num_elements - 1>
        inline void decrement() {
            --get<N>();
            if constexpr (N != 0) {
                return decrement<N-1>();
            }
        }
        //add a constant offset
        template <unsigned N, class T>
        inline void add(const T& x) {
            get<N>() += x;
            if constexpr (N != 0) {
                return add<N-1>(x);
            }
        }

        //returns true if Op(*this, other) is true for any subiterator
        template <class Op, unsigned N = num_elements - 1>
        inline bool any(const iterator_t& other, Op o = Op{}) const {
            if (o(get<N>(), other.get<N>())) {
                return true;
            }
            if constexpr (N != 0) {
                return any<Op, N-1>(other, o);
            }
            else return false;
        }

        //returns true if Op(*this, other) is true for all subiterators
        template <class Op, unsigned N = num_elements - 1>
        inline bool all(const iterator_t& other, Op o = Op{}) const {
            if (!o(get<N>(), other.get<N>())) {
                return false;
            }
            if constexpr (N != 0) {
                return all<Op, N-1>(other, o);
            }
            else return true;
        }

        //this function is a bit uglier than it needs to be because we
        //end up fighting the template argument deduction rules
        //
        //essentially, T is an accumulator and we just continue
        //concatenating the dereferenced types on to the end
        template <class T, unsigned N, class First, class... Rest>
        inline auto deref(T&& t) const {
            auto x = std::tuple_cat(std::move(t), 
                    std::tuple<subiter_ref_t<N>>{*get<N>()});
            return deref<decltype(x), N+1, Rest...>(std::move(x));
        }
        template <class T, unsigned N>
        inline auto deref(T&& t) const {
            return std::move(t);
        }
    public:
        iterator_t() = default;
        ~iterator_t() = default;
        iterator_t(const iterator_t&) = default;
        iterator_t(iterator_t&&) = default;
        iterator_t& operator=(const iterator_t&) = default;
        iterator_t& operator=(iterator_t&&) = default;
        
        using value_type = std::tuple<std::ranges::range_reference_t<R>...>;
        using reference_type = std::tuple<std::ranges::range_reference_t<R>...>;
        using difference_type = std::ptrdiff_t;

        iterator_t& operator++() {
            increment();
            return *this;
        }
        iterator_t operator++(int) {
            iterator_t ret{*this};
            ++(*this);
            return ret;
        }
        // Only provide decrementing if it's supported
        template <class I = iterator_t>
        typename std::enable_if<I::bidirectional,
        iterator_t&>::type operator--() {
            decrement();
            return *this;
        }
        template <class I = iterator_t>
        typename std::enable_if<I::bidirectional,
        iterator_t&>::type operator--(int) {
            iterator_t ret{*this};
            --(*this);
            return ret;
        }
        
        reference_type operator*() const {
            return deref<std::tuple<>, 0, R...>(std::tuple<>());
        }
        reference_type operator->() const {
            return this->operator*();
        }
        
        /* Comparison operators:
         * 
         * The tricky part here is how to handle multiple ranges
         * that are not the same size.  The most important thing
         * here is for i < end() to be handled properly.
         *
         * So, if any two subiterators are equal, we'll consider
         * the zipped iterators to be equal.  This way a loop
         * that checks (i++ < end) will terminate when the
         * first subrange runs out of elements.
         *
         * For consistency then, we'll say all subiterators must
         * satisfy (a < b) for operator<, and <=/>/>= follow
         * similar logic
         */
        bool operator==(const iterator_t& other) const {
            return any<std::equal_to<>>(other);
        }
        bool operator!=(const iterator_t& other) const {
            return !(*this == other);
        }

        // Only provide </<=/>/>= if they're supported
        template <class I = iterator_t>
        typename std::enable_if<I::ordered,
        bool>::type operator<(const iterator_t& other) const {
            return all<std::less<>>(other);
        }
        template <class I = iterator_t>
        typename std::enable_if<I::ordered,
        bool>::type operator>=(const iterator_t& other) const {
            return !(*this < other);
        }
        template <class I = iterator_t>
        typename std::enable_if<I::ordered,
        bool>::type operator>(const iterator_t& other) const {
            return !(*this == other) && any<std::greater<>>(other);
        }
        template <class I = iterator_t>
        typename std::enable_if<I::ordered,
        bool>::type operator<=(const iterator_t& other) const {
            return !(*this > other);
        }

        // Only provide addition/subtraction if they're supported
        template <class T, class I = iterator_t>
        typename std::enable_if<I::random_access,
        iterator_t&>::type operator+=(const T& rhs) {
            add(rhs);
            return *this;
        }
        template <class T, class I = iterator_t>
        typename std::enable_if<I::random_access,
        iterator_t&>::type operator-=(const T& rhs) {
            add(-rhs);
            return *this;
        }
        template <class T, class I = iterator_t>
        typename std::enable_if<I::random_access,
        iterator_t>::type operator+(const T& rhs) const {
            iterator_t ret{*this};
            ret += rhs;
            return ret;
        }
        template <class T, class I = iterator_t>
        typename std::enable_if<I::random_access,
        iterator_t>::type operator-(const T& rhs) const {
            iterator_t ret{*this};
            ret -= rhs;
            return ret;
        }

    };

    zip_range(R... r) : ranges{r...} {}
    iterator_t begin() const {
        return iterator_t{begin_impl<std::tuple<>, 0, R...>(std::tuple<>())};
    }
    iterator_t end() const {
        return iterator_t{end_impl<std::tuple<>, 0, R...>(std::tuple<>())};
    }
};

template <class... T>
auto zip(T&&... t) {
    return zip_range<typename remove_rvalue_ref<T&&>::type...>
        {std::forward<T>(t)...};
}

template <class... Args>
constexpr bool std::ranges::enable_borrowed_range<zip_range<Args...>> = true;

#endif
