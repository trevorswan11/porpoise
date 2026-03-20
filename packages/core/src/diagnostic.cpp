#include <sstream>

#include <fmt/format.h>

#include "diagnostic.hpp"

namespace porpoise::detail {

auto format_diagnostic(const std::optional<std::string>&    message,
                       std::string_view                     error_name,
                       const std::optional<SourceLocation>& location) -> std::string {
    std::stringstream ss;
    if (message) { ss << *message << " ("; }
    ss << error_name;
    if (message) { ss << ")"; }
    if (location) { ss << fmt::format(" {}", *location); }
    return ss.str();
}

} // namespace porpoise::detail
