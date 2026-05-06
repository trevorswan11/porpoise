#pragma once

#include <algorithm>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "string.hpp"

namespace porpoise::tests::helpers {

// Checks if the error list is empty, dumping the list's contents otherwise.
template <typename E> auto check_errors(std::span<const E> errors) {
    if (!errors.empty()) { fmt::println("{}", errors); }
    CHECK(errors.empty());
}

// Checks if the error list is matches the expected, dumping the list's contents otherwise.
template <typename E, typename... Es>
auto check_errors_against(std::span<const E> errors, Es&&... expected_errors) {
    const std::array expected_arr{std::forward<Es>(expected_errors)...};
    constexpr auto   expected_count = sizeof...(Es);

    if constexpr (expected_count == 0) {
        check_errors<E>(errors);
    } else {
        if (errors.size() != expected_count) {
            for (const auto& e : errors) { fmt::println("{}", e); }
            CHECK(errors.size() == expected_count);
        }

        const auto ranges_eq = std::ranges::equal(errors, expected_arr);
        if (!ranges_eq) { fmt::println("{}", errors); }
        CHECK(ranges_eq);
    }
}

constexpr auto trim_semicolons(std::string_view str) -> std::string_view {
    return string::trim_right(str, [](byte b) { return b == ';'; });
}

template <typename T, typename... Ts> auto make_vector(Ts&&... es) -> std::vector<T> {
    std::vector<T> list;
    list.reserve(sizeof...(es));
    (list.emplace_back(std::forward<Ts>(es)), ...);
    return list;
}

// Calculates the array's combinations with itself, given by:
// [A, B, C] -> [[A, B], [A, C], [B, B]]
template <typename T, usize N> constexpr auto combinations(std::array<T, N> input) {
    constexpr auto                    size = (N * (N - 1)) / 2;
    std::array<std::pair<T, T>, size> results;
    if (input.size() < 2) { return results; }

    for (usize i = 0, idx = 0; i < input.size() - 1; ++i) {
        for (usize j = i + 1; j < input.size(); ++j) { results[idx++] = {input[i], input[j]}; }
    }
    return results;
}

} // namespace porpoise::tests::helpers
