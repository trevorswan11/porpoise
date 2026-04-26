#include <catch2/catch_test_macros.hpp>

#include "diagnostic/diagnostic.hpp"

namespace porpoise::tests {

enum class TestEnum;

TEST_CASE("Diagnostic type checkers") {
    STATIC_CHECK(is_diagnostic_v<Diagnostic<TestEnum>>);
    STATIC_CHECK_FALSE(is_diagnostic_v<TestEnum>);
}

} // namespace porpoise::tests
