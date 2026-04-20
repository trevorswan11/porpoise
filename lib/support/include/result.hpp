#pragma once

#include <expected>

namespace porpoise {

template <typename T, typename E> using Result = std::__1::expected<T, E>;
template <typename E> using Err                = std::__1::unexpected<E>;

template <typename E, typename... Args> auto make_err(Args&&... args) -> Err<E> {
    return Err<E>{E{std::forward<Args>(args)...}};
}

// A hack to imitate the 'try' keyword in zig using GNU Statement Expressions
// https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
#define TRY(expr)                                                   \
    ({                                                              \
        auto&& _e = (expr);                                         \
        if (!_e.has_value()) { return Err{std::move(_e).error()}; } \
        std::move(_e).value();                                      \
    })

} // namespace porpoise
