#pragma once

#include <cassert>
#include <cstdio>
#include <source_location>
#include <utility> // IWYU pragma: export

#include <fmt/format.h>

namespace porpoise {

#define MAKE_GETTER_2(name, ReturnType) \
    [[nodiscard]] auto get_##name() const noexcept -> ReturnType { return name##_; }
#define MAKE_GETTER_3(name, ReturnType, getter) \
    [[nodiscard]] auto get_##name() const noexcept -> ReturnType { return getter(name##_); }
#define GET_GETTER_MACRO(_1, _2, _3, NAME, ...) NAME

#define MAKE_GETTER(...) GET_GETTER_MACRO(__VA_ARGS__, MAKE_GETTER_3, MAKE_GETTER_2)(__VA_ARGS__)

#define MAKE_DEDUCING_2(name, ReturnType)                                                        \
    template <typename Self> [[nodiscard]] auto get_##name(this Self&& self) noexcept -> auto& { \
        return self.name##_;                                                                     \
    }
#define MAKE_DEDUCING_3(name, ReturnType, getter)                                                \
    template <typename Self> [[nodiscard]] auto get_##name(this Self&& self) noexcept -> auto& { \
        return getter(self.name##_);                                                             \
    }
#define GET_DEDUCING_GETTER_MACRO(_1, _2, _3, NAME, ...) NAME

#define MAKE_DEDUCING_GETTER(...) \
    GET_DEDUCING_GETTER_MACRO(__VA_ARGS__, MAKE_DEDUCING_3, MAKE_DEDUCING_2)(__VA_ARGS__)

#define MAKE_EQ_DELEGATION(T)                                                           \
    [[nodiscard]] friend auto operator==(const T& lhs, const T& rhs) noexcept -> bool { \
        return lhs.is_equal(rhs);                                                       \
    }                                                                                   \
                                                                                        \
  private:                                                                              \
    auto is_equal(const T&) const noexcept -> bool;

#define MAKE_MOVE_CONSTRUCTABLE_ONLY(Type)        \
    Type(const Type&)                  = delete;  \
    auto operator=(const Type&)->Type& = delete;  \
    Type(Type&&) noexcept              = default; \
    auto operator=(Type&&)->Type&      = delete;

namespace detail {

template <typename... Args>
auto todo_impl(std::source_location loc, [[maybe_unused]] Args&&... args) noexcept -> void {
    fmt::println(stderr, "TODO: {}:{}:{}", loc.file_name(), loc.line(), loc.column());
    assert(false && "TODO");
}

} // namespace detail

#define TODO(...)                                                                \
    ::porpoise::detail::todo_impl(std::source_location::current(), __VA_ARGS__); \
    std::unreachable()

#define MAKE_UNALIASED_ITERATOR(Container, member)                                         \
    using iterator       = typename Container::iterator;                                   \
    using const_iterator = typename Container::const_iterator;                             \
                                                                                           \
    [[nodiscard]] auto begin() noexcept -> iterator { return member.begin(); }             \
    [[nodiscard]] auto end() noexcept -> iterator { return member.end(); }                 \
                                                                                           \
    [[nodiscard]] auto begin() const noexcept -> const_iterator { return member.begin(); } \
    [[nodiscard]] auto end() const noexcept -> const_iterator { return member.end(); }     \
                                                                                           \
    [[nodiscard]] auto size() const noexcept -> usize { return member.size(); }            \
    [[nodiscard]] auto empty() const noexcept -> bool { return member.empty(); }

#define MAKE_ITERATOR(Alias, Container, member) \
    using Alias = Container;                    \
    MAKE_UNALIASED_ITERATOR(Alias, member)

} // namespace porpoise
