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
    // The source and location play nicely with one another
    if (source_path) {
        fmt::print(ss, "{}:", *source_path);
        if (location) {
            fmt::print(ss, "{}: ", *location);
        } else {
            fmt::print(ss, " ", *location);
        }
    }

    // The optional message changes position based on source path presence
    if (message) { ss << *message << " ("; }
    ss << error_name;
    if (message) { ss << ")"; }
    if (!source_path && location) { ss << fmt::format(" {}", *location); }
    return ss.str();
}

} // namespace porpoise::detail
