#pragma once

#include <string_view>

#include "types.hpp"

namespace porpoise::string {

using Predicate = bool (*)(byte);

[[nodiscard]] auto is_space(byte b) noexcept -> bool;

[[nodiscard]] auto trim_left(std::string_view str, Predicate pred = is_space) noexcept
    -> std::string_view;

// Same as `trim_left` but counts the number of times the predicate was true
[[nodiscard]] auto trim_left(std::string_view str, usize& count, Predicate pred = is_space) noexcept
    -> std::string_view;

[[nodiscard]] auto trim_right(std::string_view str, Predicate pred = is_space) noexcept
    -> std::string_view;

// Same as `trim_right` but counts the number of times the predicate was true
[[nodiscard]] auto trim_right(std::string_view str,
                              usize&           count,
                              Predicate        pred = is_space) noexcept -> std::string_view;

[[nodiscard]] auto trim(std::string_view str, Predicate pred = is_space) noexcept
    -> std::string_view;

// Same as `trim` but counts the number of times the predicate was true
[[nodiscard]] auto trim(std::string_view str, usize& count, Predicate pred = is_space) noexcept
    -> std::string_view;

// Zero allocation substring returning empty substring for invalid input
[[nodiscard]] auto substr(std::string_view str,
                          usize            pos,
                          usize len = std::string_view::npos) noexcept -> std::string_view;

// Think hard about why a view of an rvalue temporary string is a bad idea
auto substr(std::string&& str, usize pos, usize len = std::string_view::npos) noexcept
    -> std::string_view = delete;

} // namespace porpoise::string
