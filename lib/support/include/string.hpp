#pragma once

#include <string_view>

#include "types.hpp"

namespace porpoise::string {

using Predicate = bool (*)(byte);

[[nodiscard]] auto is_space(byte b) noexcept -> bool;

[[nodiscard]] auto trim_left(std::string_view str, Predicate pred = is_space) noexcept
    -> std::string_view;
[[nodiscard]] auto trim_right(std::string_view str, Predicate pred = is_space) noexcept
    -> std::string_view;
[[nodiscard]] auto trim(std::string_view str, Predicate pred = is_space) noexcept
    -> std::string_view;

} // namespace porpoise::string
