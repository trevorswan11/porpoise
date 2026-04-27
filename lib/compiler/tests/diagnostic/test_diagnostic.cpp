#include <catch2/catch_test_macros.hpp>

#include "diagnostic/diagnostic.hpp"

namespace porpoise {

struct SomethingLocationed {};

template <> struct SourceInfo<SomethingLocationed> {
    static auto get(const SomethingLocationed&) noexcept -> SourceLocation { return {0, 42}; }
};

struct SomethingElseLocationed {};

template <> struct SourceInfo<SomethingElseLocationed> {
    static auto get(const SomethingElseLocationed&) noexcept -> SourceLocation { return {42, 0}; }
};

namespace tests {

enum class TestEnum {
    SAD,
    MAD,
};

TEST_CASE("Diagnostic type checkers") {
    STATIC_CHECK(is_diagnostic_v<Diagnostic<TestEnum>>);
    STATIC_CHECK_FALSE(is_diagnostic_v<TestEnum>);
}

TEST_CASE("Custom locateable") {
    SomethingLocationed        l;
    const Diagnostic<TestEnum> d{TestEnum::SAD, l};
    CHECK("SAD 0:42" == d.to_string());
}

TEST_CASE("Error messages with associated files") {
    const Diagnostic<TestEnum> d{TestEnum::SAD};
    CHECK("foo.porp: SAD" == d.to_string("foo.porp"));
}

TEST_CASE("Locateable Error messages with associated files") {
    SomethingLocationed        l;
    const Diagnostic<TestEnum> d{TestEnum::SAD, l};
    CHECK("foo.porp:0:42: SAD" == d.to_string("foo.porp"));
}

TEST_CASE("Move constructor with new error") {
    SomethingLocationed  l;
    Diagnostic<TestEnum> d1{TestEnum::SAD, l};
    Diagnostic<TestEnum> d2{std::move(d1), TestEnum::MAD};
    CHECK("MAD 0:42" == d2.to_string());
}

TEST_CASE("Move constructor with new location") {
    SomethingLocationed  l;
    Diagnostic<TestEnum> d1{TestEnum::SAD, l};

    SomethingElseLocationed e;
    Diagnostic<TestEnum>    d2{std::move(d1), e};
    CHECK("SAD 42:0" == d2.to_string());
}

} // namespace tests

} // namespace porpoise
