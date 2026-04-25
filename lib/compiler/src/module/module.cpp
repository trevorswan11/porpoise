#include <fmt/format.h>

#include "syntax/parser.hpp"

#include "module/module.hpp"

#include "array.hpp"

namespace porpoise::mod {

auto ModuleManager::try_get(const std::filesystem::path& path)
    -> Result<opt::NonNull<Module>, Diagnostic> {
    const auto abs_path     = std::filesystem::absolute(path).lexically_normal();
    const auto abs_path_str = abs_path.string();

    // Prevent re-parsing by checking the map, safe as pointers are stable
    if (auto it = modules_.find(abs_path_str); it != modules_.end()) { return it->second.get(); }
    auto source = loader_.load(abs_path_str);
    if (!source) {
        return make_module_err(fmt::format("Could not load file: {}", abs_path_str),
                               source.error());
    }

    syntax::Parser p{*source};
    auto [ast, diagnostics] = p.consume(abs_path_str);

    auto mod =
        mem::make_box<Module>(abs_path,
                              std::move(*source),
                              std::move(ast),
                              array::Index{},
                              diagnostics.empty() ? ModuleState::PARSED : ModuleState::ERRORED,
                              std::move(diagnostics));
    auto* ptr = mod.get();
    modules_.emplace(abs_path_str, std::move(mod));
    return ptr;
}

} // namespace porpoise::mod
