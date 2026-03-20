#pragma once

#include <utility> // IWYU pragma: export
#include <variant> // IWYU pragma: export

template <class... Ts> struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

#define MAKE_VARIANT_UNPACKER(name, ReturnType, InnerType, member, getter) \
    [[nodiscard]] auto get_##name() const noexcept -> const ReturnType& {  \
        try {                                                              \
            return getter<InnerType>(member);                              \
        } catch (...) { std::unreachable(); }                              \
    }                                                                      \
                                                                           \
    [[nodiscard]] auto is_##name() const noexcept -> bool {                \
        return std::holds_alternative<InnerType>(member);                  \
    }

// Provides std::visit-like access to the internal node
#define MAKE_VARIANT_MATCHER(member)                               \
    template <class Matcher> auto match(Matcher&& matcher) {       \
        return std::visit(std::forward<Matcher>(matcher), member); \
    }                                                              \
    template <class Matcher> auto match(Matcher&& matcher) const { \
        return std::visit(std::forward<Matcher>(matcher), member); \
    }
