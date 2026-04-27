#include <fmt/format.h>
#include <fmt/ostream.h>

#include "diagnostic/diagnostic.hpp"

namespace porpoise::detail {

auto format_diagnostic(std::ostream&                   os,
                       FormattableDiagnostic&&         diag,
                       const opt::Option<std::string>& source_path) -> std::ostream& {
    // The source and location play nicely with one another
    if (source_path) {
        fmt::print(os, "{}:", *source_path);
        if (diag.location) {
            fmt::print(os, "{}: ", *diag.location);
        } else {
            fmt::print(os, " ");
        }
    }

    // The optional message changes position based on source path presence
    if (diag.message) { os << *diag.message << " ("; }
    os << diag.error_name;
    if (diag.message) { os << ")"; }
    if (!source_path && diag.location) { os << fmt::format(" {}", *diag.location); }
    return os;
}

} // namespace porpoise::detail
