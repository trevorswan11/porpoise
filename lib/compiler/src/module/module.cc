#include <fmt/format.h>

#include "syntax/parser.hh"

#include "module/module.hh"

#include "diagnostic.hh"

namespace porpoise::mod {

auto format_module_diagnostic(std::ostream&                   os,
                              detail::FormattableDiagnostic&& diag,
                              opt::Option<const mod::Module&> module,
                              opt::Option<bool>               in_terminal) -> std::ostream& {
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

auto Module::print_diagnostics(std::ostream& os) const -> void {
    if (is_ok()) { return; }
    match(Overloaded{
        [&](const auto& l) {
            for (const auto& diag : l) {
                format_module_diagnostic(os, diag.to_formattable(), *this, l.get_terminal_status())
                    << "\n";
            }
        },
        [](const Unit&) { std::unreachable(); }});
}

auto ModuleManager::try_get_file_module(const std::filesystem::path& path,
                                        const std::filesystem::path& parent_path)
    -> Result<mem::NonNull<Module>, Diagnostic> {
    ASSERT((parent_path.empty() || parent_path.is_absolute()) &&
           "Parent path must be absolute or empty");
    if (!path.is_relative()) {
        return make_mod_err(fmt::format("Requested file '{}' is absolute", path.string()),
                            Error::MODULE_PATH_NOT_RELATIVE);
    };

    const auto normalized = loader_.normalize(parent_path.empty() ? path : parent_path / path);
    if (!normalized) { return make_mod_err(normalized.error()); }
    return try_get(*normalized);
}

auto ModuleManager::try_get_library_module(const std::string& name)
    -> Result<mem::NonNull<Module>, Diagnostic> {
    auto it = module_lut_.find(name);
    if (it == module_lut_.end()) {
        return make_mod_err(fmt::format("Unknown module '{}'", name), Error::MODULE_DOES_NOT_EXIST);
    }
    return try_get(it->second);
}

auto ModuleManager::add_library_module(const std::string& name, const std::filesystem::path& path)
    -> Result<Unit, Diagnostic> {
    const auto normalized = loader_.normalize(path);
    if (!normalized) { return make_mod_err(normalized.error()); }

    if (auto it = module_lut_.find(name); it != module_lut_.end()) {
        if (it->second == normalized) { return Unit{}; }
        return make_mod_err(fmt::format("Attempt to add duplicate module '{}' from path '{}' "
                                        "which already exists at path '{}'",
                                        name,
                                        path.string(),
                                        it->second.string()),
                            Error::MODULE_ALREADY_EXISTS);
    }

    module_lut_.emplace(name, *normalized);
    return Unit{};
}

auto ModuleManager::try_get(const std::filesystem::path& path)
    -> Result<mem::NonNull<Module>, Diagnostic> {
    // Prevent re-parsing by checking the map, safe as pointers are stable
    if (auto it = modules_.find(path); it != modules_.end()) { return it->second.get(); }
    auto       source       = TRY(loader_.load(path));
    const auto abs_path_str = path.string();

    auto mod =
        mem::make_box<Module>(path, path.parent_path(), SourceFile{std::move(source)}, ast::AST{});
    syntax::Parser p{mod->source};
    auto           diagnostics = p.consume(mod->tree);

    mod->state       = diagnostics.empty() ? ModuleState::PARSED : ModuleState::ERRORED;
    mod->diagnostics = std::move(diagnostics);

    auto* ptr = mod.get();
    modules_.emplace(path, std::move(mod));
    return ptr;
}

} // namespace porpoise::mod
