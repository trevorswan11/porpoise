#include "diagnostic/list.hpp"

#include "sema/module/module.hpp"

namespace porpoise::detail {

auto format_module_diagnostic(std::ostream&                         os,
                              FormattableDiagnostic&&               diag,
                              opt::Option<const sema::mod::Module&> module) -> std::ostream& {
    // Without a module, there is no source path and formatting is done trivially
    if (!module) { return format_diagnostic(os, std::move(diag), opt::none); }

    // Without location, there's no way to point to an error
    format_diagnostic(os, std::move(diag), module->path.string());
    if (!diag.location) { return os; }

    // The source location is guaranteed to be present here
    return os;
}

} // namespace porpoise::detail
