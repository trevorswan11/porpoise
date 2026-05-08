#include <catch2/catch_test_macros.hpp>

#include "helpers.hh"

#include "memory.hh"
#include "static_vector.hh"

namespace porpoise::tests {

TEST_CASE("StaticVector type checks") {
    STATIC_REQUIRE(TriviallyDestructible<StaticVector<mem::NonNull<i32>, 4>>);
    STATIC_REQUIRE_FALSE(TriviallyDestructible<StaticVector<mem::Box<i32>, 4>>);
}

TEST_CASE("StaticVector basic usage") {
    StaticVector<i32, 5> vec;
    CHECK(vec.empty());
    CHECK(vec.size() == 0);

    for (i32 i = 0; i < 5; ++i) { vec.push_back(i * 10); }
    CHECK(vec.size() == 5);
    CHECK_FALSE(vec.empty());

    CHECK_THROWS_AS(vec.push_back(10), std::out_of_range);
}

TEST_CASE("StaticVector iteration") {
    StaticVector<i32, 5> vec{1, 2, 3};
    i32                  count = 0;
    i32                  sum   = 0;
    for (i32 val : vec) {
        sum += val;
        count++;
    }

    CHECK(sum == 6);
    CHECK(count == 3);
    CHECK(vec.end() == vec.begin() + count);
}

TEST_CASE("StaticVector indexing") {
    StaticVector<i32, 3> vec{10, 20, 30};
    CHECK(vec[0] == 10);
    CHECK(vec[1] == 20);
    CHECK(vec[2] == 30);
}

TEST_CASE("StaticVector with non-trivial type") {
    SECTION("Contrived example") {
        struct Point {
            i32 x, y;
            Point(i32 x, i32 y) : x{x}, y{y} {}
        };

        StaticVector<Point, 2> points;
        points.emplace_back(5, 10);
        CHECK(points[0].x == 5);
        CHECK(points[0].y == 10);
    }

    SECTION("NonNull usage") {
        StaticVector<mem::NonNull<i32>, 2> ptrs;
        i32                                v = 22;
        ptrs.emplace_back(&v);
        CHECK(ptrs[0] == &v);
        CHECK(*ptrs[0] == v);
    }
}

TEST_CASE("StaticVector span conversion") {
    StaticVector<i32, 4> vec{1, 2};
    std::span<i32>       s = vec;
    CHECK(s.size() == 2);
    CHECK(std::ranges::equal(s, vec));
}

using Tracker = helpers::RAIITracker;

TEST_CASE("StaticVector destructor correctness") {
    Tracker::reset();
    {
        StaticVector<Tracker, 5> vec;
        vec.emplace_back();
        vec.emplace_back();
    }
    CHECK(Tracker::destruct_count == 2);
}

TEST_CASE("StaticVector copy correctness") {
    Tracker::reset();
    {
        StaticVector<Tracker, 3> original;
        original.emplace_back();
        original.emplace_back();

        SECTION("Copy constructor") {
            StaticVector<Tracker, 3> copy = original;
            CHECK(copy.size() == 2);
            CHECK(Tracker::copy_count == 2);

            // 2 in original, 2 in copy
            CHECK(Tracker::live_count == 4);
        }

        SECTION("Copy assignment") {
            StaticVector<Tracker, 3> assigned;
            assigned = original;
            CHECK(assigned.size() == 2);

            // Copy internally followed by move
            CHECK(Tracker::copy_count == 2);
        }
    }
    CHECK(Tracker::live_count == 0);
}

TEST_CASE("StaticVector move correctness") {
    Tracker::reset();
    {
        StaticVector<Tracker, 3> original;
        original.emplace_back();
        original.emplace_back();

        StaticVector<Tracker, 3> destination = std::move(original);
        CHECK(destination.size() == 2);
        CHECK(original.empty());
        CHECK(Tracker::move_count == 2);
        CHECK(Tracker::live_count == 2);
    }
    CHECK(Tracker::live_count == 0);
}

TEST_CASE("Self assignment") {
    Tracker::reset();
    StaticVector<Tracker, 3> vec;
    vec.emplace_back();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
    vec = vec;
#pragma clang diagnostic pop

    CHECK(vec.size() == 1);
    CHECK(Tracker::live_count == 1);
}

} // namespace porpoise::tests
