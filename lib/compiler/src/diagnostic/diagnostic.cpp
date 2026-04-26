#include <sstream>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "diagnostic/diagnostic.hpp"

namespace porpoise::detail {

auto format_diagnostic(const opt::Option<std::string>&    message,
                       std::string_view                   error_name,
                       const opt::Option<std::string>&    source_path,
                       const opt::Option<SourceLocation>& location) -> std::string {
    std::stringstream ss;
    if (message) { ss << *message << " ("; }
    ss << error_name;
    if (message) { ss << ")"; }

    // The source and location play nicely with one another
    if (source_path) { fmt::print(ss, " {}", *source_path); }
    if (location) { fmt::print(ss, "{}{}", source_path ? ":" : " ", *location); }
    return ss.str();
}

} // namespace porpoise::detail
