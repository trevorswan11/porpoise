#include <algorithm>
#include <cassert>
#include <cctype>
#include <ranges>

#include "string.hpp"

namespace porpoise::string {

auto is_space(byte b) noexcept -> bool { return std::isspace(b); }

auto trim_left(std::string_view str, Predicate pred) noexcept -> std::string_view {
    const auto first = std::ranges::find_if_not(str, pred);
    return std::string_view{first, static_cast<usize>(str.end() - first)};
}

auto trim_left(std::string_view str, usize& count, Predicate pred) noexcept -> std::string_view {
    const auto ltrim = trim_left(str, pred);
    count += str.size() - ltrim.size();
    return ltrim;
}

auto trim_right(std::string_view str, Predicate pred) noexcept -> std::string_view {
    const auto last = std::ranges::find_if_not(str | std::views::reverse, pred).base();
    return std::string_view{str.begin(), last};
}

auto trim_right(std::string_view str, usize& count, Predicate pred) noexcept -> std::string_view {
    const auto rtrim = trim_right(str, pred);
    count += str.size() - rtrim.size();
    return rtrim;
}

auto trim(std::string_view str, Predicate pred) noexcept -> std::string_view {
    const auto ltrim = trim_left(str, pred);
    return trim_right(ltrim, pred);
}

auto trim(std::string_view str, usize& count, Predicate pred) noexcept -> std::string_view {
    const auto trimmed = trim(str, pred);
    count += str.size() - trimmed.size();
    return trimmed;
}

auto substr(std::string_view str, usize pos, usize len) noexcept -> std::string_view {
    return pos > str.size() ? std::string_view{}
                            : std::string_view{str.data() + pos, std::min(len, str.size() - pos)};
}

} // namespace porpoise::string
