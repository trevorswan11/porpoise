#pragma once

#include "types.hh"

namespace porpoise::tests::helpers {

struct Base {
    virtual ~Base() = default;
    i32 x           = 10;
};

struct Derived : Base {
    i32 y = 20;
};

} // namespace porpoise::tests::helpers
