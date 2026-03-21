#include <numbers>
#include <variant>

#include <catch2/catch_test_macros.hpp>

#include "variant.hpp"

namespace porpoise::tests {

TEST_CASE("Variant visitor") {
    std::variant<std::string, int, double> one_of_many;
    const std::string                      expected_string{"Hello!"};
    const int                              expected_int{42};
    const double                           expected_double{std::numbers::pi};

    const auto visitor = Overloaded{
        [&expected_string](const std::string& s) { return s == expected_string; },
        [](int i) { return i == expected_int; },
        [&expected_double](double d) { return d == expected_double; },
    };

    one_of_many = expected_string;
    CHECK(std::visit(visitor, one_of_many));
    one_of_many = expected_int;
    CHECK(std::visit(visitor, one_of_many));
    one_of_many = expected_double;
    CHECK(std::visit(visitor, one_of_many));
}

} // namespace porpoise::tests
