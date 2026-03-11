#pragma once

#include <cassert>
#include <cstdio>
#include <source_location>
#include <utility> // IWYU pragma: export

#include <fmt/format.h>

namespace conch {

namespace detail {

template <typename... Args>
auto todo_impl(std::source_location loc, [[maybe_unused]] Args&&... args) noexcept -> void {
    fmt::println(stderr, "TODO: {}:{}:{}", loc.file_name(), loc.line(), loc.column());
    assert(false && "TODO");
}

} // namespace detail

#define TODO(...)                                                             \
    ::conch::detail::todo_impl(std::source_location::current(), __VA_ARGS__); \
    std::unreachable()

} // namespace conch
