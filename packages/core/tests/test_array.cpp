#include <ranges>

#include <catch2/catch_test_macros.hpp>

#include "array.hpp"

namespace porpoise::tests {

TEST_CASE("View materialization") {
    constexpr auto nums         = std::ranges::views::iota(0, 100);
    constexpr auto materialized = array::materialize_sized_view<100>(nums);
    for (const auto num : nums) { CHECK(materialized[num] == num); }
}

TEST_CASE("Array concatenation") {
    constexpr auto               A = std::array{0, 1, 2, 3, 4, 5, 6};
    constexpr auto               B = std::array{7, 8};
    constexpr std::array<i32, 0> C = {};

    constexpr auto combined = array::concat(A, B, C);
    for (usize i = 0; i < combined.size(); ++i) { CHECK(combined[i] == static_cast<i32>(i)); }
}

TEST_CASE("Array combinations") {
    constexpr auto A        = std::array{0, 1, 2};
    constexpr auto expected = std::to_array<std::pair<i32, i32>>({{0, 1}, {0, 2}, {1, 2}});
    constexpr auto actual   = array::combinations<i32>(A);
    CHECK(std::ranges::equal(expected, actual));
}

} // namespace porpoise::tests
