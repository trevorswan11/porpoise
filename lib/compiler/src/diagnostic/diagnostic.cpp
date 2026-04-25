#include <sstream>

#include <fmt/format.h>

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
    if (source_path) {
        ss << *source_path << ":";
    } else {
        ss << " ";
    }

    if (location) { ss << fmt::format("{}", *location); }
    return ss.str();
}

} // namespace porpoise::detail
