#include <cctype>
#include <ranges>

#include "string.hpp"

namespace porpoise::string {

auto is_space(byte b) noexcept -> bool { return std::isspace(b); }

auto trim_left(std::string_view str, Predicate pred) noexcept -> std::string_view {
    const auto first = std::ranges::find_if_not(str, pred);
    return std::string_view{first, static_cast<usize>(str.end() - first)};
}

auto trim_right(std::string_view str, Predicate pred) noexcept -> std::string_view {
    const auto last = std::ranges::find_if_not(str | std::views::reverse, pred).base();
    return std::string_view{str.begin(), last};
}

auto trim(std::string_view str, Predicate pred) noexcept -> std::string_view {
    const auto ltrim = trim_left(str, pred);
    return trim_right(ltrim, pred);
}

} // namespace porpoise::string
