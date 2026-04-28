#include "diagnostic/list.hpp"

#include "sema/module/module.hpp"

#include "utility.hpp"

namespace porpoise::detail {

auto format_module_diagnostic(std::ostream&                         os,
                              FormattableDiagnostic&&               diag,
                              opt::Option<const sema::mod::Module&> module,
                              opt::Option<bool>                     in_terminal) -> std::ostream& {
    const auto tty = in_terminal.value_or(is_tty());

    // Without a module, there is no source path and formatting is done trivially
    if (!module) { return format_diagnostic(os, std::move(diag), opt::none, tty); }

    // Without location, there's no way to point to an error
    format_diagnostic(os, std::move(diag), module->path.string(), tty);
    if (!diag.location) { return os; }

    // Diagnostic error messages can include the location
    const auto [line, caret] = module->source.get_diagnostic_strings(*diag.location);
    fmt::print(os, "\n    {}", line);
    if (caret) { os << fmt::format(tty ? style::CARET : style::BASE, "\n    {}", *caret); }
    return os;
}

} // namespace porpoise::detail
