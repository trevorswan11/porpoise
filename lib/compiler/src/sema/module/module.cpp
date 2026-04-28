#include <fmt/format.h>

#include "sema/module/module.hpp"

namespace porpoise::sema::mod {

auto Module::print_diagnostics(std::ostream& os) const -> void {
    if (!is_errored()) { return; }
    match([&](const auto& l) { l.print(os, *this); });
}

auto ModuleManager::try_get_file_module(const std::filesystem::path& path,
                                        const std::filesystem::path& parent_path)
    -> Result<opt::NonNull<Module>, Diagnostic> {
    assert((parent_path.empty() || parent_path.is_absolute()) &&
           "Parent path must be absolute or empty");
    if (!path.is_relative()) {
        return make_sema_err(fmt::format("Requested file '{}' is absolute", path.string()),
                             Error::IMPORT_NOT_RELATIVE);
    };

    const auto normalized = loader_.normalize(parent_path.empty() ? path : parent_path / path);
    if (!normalized) { return make_sema_err(normalized.error()); }
    return try_get(*normalized);
}

auto ModuleManager::try_get_library_module(const std::string& name)
    -> Result<opt::NonNull<Module>, Diagnostic> {
    auto it = module_lut_.find(name);
    if (it == module_lut_.end()) {
        return make_sema_err(fmt::format("Unknown module '{}'", name),
                             Error::MODULE_DOES_NOT_EXIST);
    }
    return try_get(it->second);
}

auto ModuleManager::add_porpoise_module(const std::string& name, const std::filesystem::path& path)
    -> Result<Unit, Diagnostic> {
    const auto normalized = loader_.normalize(path);
    if (!normalized) { return make_sema_err(normalized.error()); }

    if (auto it = module_lut_.find(name); it != module_lut_.end()) {
        if (it->second == normalized) { return Unit{}; }
        return make_sema_err(
            fmt::format(
                "Attempt to add duplicate module {} from {} which already exists at path {}",
                name,
                normalized->string(),
                it->second.string()),
            Error::MODULE_ALREADY_EXISTS);
    }

    module_lut_.emplace(name, *normalized);
    return Unit{};
}

auto ModuleManager::try_get(const std::filesystem::path& path)
    -> Result<opt::NonNull<Module>, Diagnostic> {
    // Prevent re-parsing by checking the map, safe as pointers are stable
    if (auto it = modules_.find(path); it != modules_.end()) { return it->second.get(); }
    auto       source       = loader_.load(path);
    const auto abs_path_str = path.string();
    if (!source) {
        return make_sema_err(fmt::format(R"(Could not load file: "{}")", abs_path_str),
                             source.error());
    }

    auto mod = mem::make_box<Module>(path, path.parent_path(), SourceFile{std::move(*source)});
    syntax::Parser p{mod->source};
    auto [ast, diagnostics] = p.consume();

    mod->tree        = std::move(ast);
    mod->state       = diagnostics.empty() ? ModuleState::PARSED : ModuleState::ERRORED;
    mod->diagnostics = std::move(diagnostics);

    auto* ptr = mod.get();
    modules_.emplace(path, std::move(mod));
    return ptr;
}

} // namespace porpoise::sema::mod
