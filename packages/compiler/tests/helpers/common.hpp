#pragma once

#include <span>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "string.hpp"

namespace porpoise::tests::helpers {

// Checks if the error list is empty, dumping the list's contents otherwise.
template <typename E> auto check_errors(std::span<const E> errors) {
    if (!errors.empty()) { fmt::println("{}", errors); }
    REQUIRE(errors.empty());
}

constexpr auto trim_semicolons(std::string_view str) -> std::string_view {
    return string::trim_right(str, [](byte b) { return b == ';'; });
}

} // namespace porpoise::tests::helpers
