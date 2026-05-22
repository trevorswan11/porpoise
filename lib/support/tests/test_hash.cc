#include <catch2/catch_test_macros.hpp>

#include "hash.hh"

namespace porpoise::tests {

TEST_CASE("Order dependent hashing") {
    hash::Hasher h1{1}, h2{2};
    h1.combine(2);
    h2.combine(1);
    CHECK_FALSE(h1.finalize() == h2.finalize());
}

} // namespace porpoise::tests
