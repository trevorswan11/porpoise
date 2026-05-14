#pragma once

#include <tuple>
#include <type_traits>

namespace porpoise {

// Similar to a std::pair, but the Visitor may be a function pointer
template <typename Iterable, typename Visitor> struct IterPair {
    const Iterable& iterable;
    Visitor         visitor;
};

namespace traits {

template <typename Self, typename T>
using data_pointer_t =
    std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const T*, T*>;

template <typename T>
concept InsertablePair = requires {
    typename std::tuple_element_t<0, std::remove_cvref_t<T>>;
    typename std::tuple_element_t<1, std::remove_cvref_t<T>>;
    requires std::tuple_size_v<std::remove_cvref_t<T>> >= 2;
};

} // namespace traits

#define MAKE_UNALIASED_ITERATOR(Container, member)                                               \
    using iterator       = typename Container::iterator;                                         \
    using const_iterator = typename Container::const_iterator;                                   \
                                                                                                 \
    [[nodiscard]] constexpr auto begin() noexcept -> iterator { return member.begin(); }         \
    [[nodiscard]] constexpr auto end() noexcept -> iterator { return member.end(); }             \
                                                                                                 \
    [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator {                      \
        return member.begin();                                                                   \
    }                                                                                            \
    [[nodiscard]] constexpr auto end() const noexcept -> const_iterator { return member.end(); } \
                                                                                                 \
    [[nodiscard]] constexpr auto size() const noexcept -> usize { return member.size(); }        \
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return member.empty(); }

#define MAKE_ITERATOR(Alias, Container, member) \
    using Alias = Container;                    \
    MAKE_UNALIASED_ITERATOR(Alias, member)

} // namespace porpoise
