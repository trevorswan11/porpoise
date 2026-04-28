#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "diagnostic/diagnostic.hpp"

#include "utility.hpp"

namespace porpoise::detail {

auto format_diagnostic(std::ostream&                   os,
                       FormattableDiagnostic&&         diag,
                       const opt::Option<std::string>& source_path,
                       opt::Option<bool>               in_terminal) -> std::ostream& {
    const auto tty = in_terminal.value_or(is_tty());

    // The source and location play nicely with one another
    if (source_path) {
        const auto& local_style = tty ? style::SOURCE : style::BASE;
        os << fmt::format(local_style, "{}:", *source_path);
        if (diag.location) {
            os << fmt::format(local_style, "{}: ", *diag.location);
        } else {
            fmt::print(os, " ");
        }
    }

    // Here the buffer should be "file:loc: " to print level if present
    if (diag.level) {
        const auto name = level_name(*diag.level);
        os << fmt::format(tty ? level_style(*diag.level) : style::BASE, "{}:", name);
    }

    // The optional message changes position based on source path presence
    if (diag.message) { fmt::print(os, " {}", *diag.message); }
    if (!source_path && diag.location) { os << fmt::format(" {}", *diag.location); }
    return os;
}

} // namespace porpoise::detail
