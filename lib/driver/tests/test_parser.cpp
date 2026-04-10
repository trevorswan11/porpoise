#include <initializer_list>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "arguments/parser.hpp"

namespace porpoise::tests {

namespace helpers {

class MockArgv {
  public:
    explicit MockArgv(std::initializer_list<std::string> args) : strings_(args) {
        pointers_.reserve(strings_.size() + 1);
        for (auto& s : strings_) { pointers_.push_back(s.data()); }
        pointers_.push_back(nullptr);
    }

    auto argc() const noexcept -> i32 { return static_cast<i32>(strings_.size()); }
    auto argv() noexcept -> byte** { return pointers_.data(); }

  private:
    std::vector<std::string> strings_;
    std::vector<byte*>       pointers_;
};

} // namespace helpers

TEST_CASE("Ast dump parser") {
    auto           args = helpers::MockArgv{{"porpoise", "ast"}};
    driver::Parser parser{args.argc(), args.argv()};
    CHECK(std::holds_alternative<driver::AstDump>(parser.get_parsed()));
}

} // namespace porpoise::tests
