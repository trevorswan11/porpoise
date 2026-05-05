#pragma once

#include <exception>
#include <iostream>
#include <source_location>

#include <fmt/ostream.h>

namespace porpoise {

namespace detail {

template <typename T>
constexpr auto assert_impl(std::source_location loc, const T& condition, std::string_view message)
    -> void {
    if (!condition) {
        if consteval { // cppcheck-suppress syntaxError
            fmt::println(
                std::cerr, "{}: {}:{}:{}", message, loc.file_name(), loc.line(), loc.column());
            std::terminate();
        } else {
            throw "Compile-time assertion failed";
        }
    }
}

} // namespace detail

#ifndef NDEBUG
#    define ASSERT_1(expression) \
        ::porpoise::detail::assert_impl(std::source_location::current(), expression, #expression)
#    define ASSERT_2(expression, message) \
        ::porpoise::detail::assert_impl(std::source_location::current(), expression, message)
#else
#    define ASSERT_1(expression)                         \
        do {                                             \
            if constexpr (false) { (void)(expression); } \
        } while (0)
#    define ASSERT_2(expression, message) ASSERT_1(expression)
#endif

#define GET_ASSERT_MACRO(_1, _2, NAME, ...) NAME
#define ASSERT(...) GET_ASSERT_MACRO(__VA_ARGS__, ASSERT_2, ASSERT_1)(__VA_ARGS__)

} // namespace porpoise
