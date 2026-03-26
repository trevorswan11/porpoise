#pragma once

#include <utility> // IWYU pragma: export
#include <variant> // IWYU pragma: export

template <class... Ts> struct Overloaded : Ts... {
    using Ts::operator()...;
};

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
#define MAKE_VARIANT_MATCHER(member)                                                          \
    template <typename Self, class Matcher> auto match(this Self&& self, Matcher&& matcher) { \
        return std::visit(std::forward<Matcher>(matcher), self.member);                       \
    }
