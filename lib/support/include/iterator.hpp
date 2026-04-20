#pragma once

namespace porpoise {

// Similar to a std::pair, but the Visitor may be a function pointer
template <typename Iterable, typename Visitor> struct IterPair {
    const Iterable& iterable;
    Visitor         visitor;
};

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
