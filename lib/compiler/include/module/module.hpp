#pragma once

#include <filesystem>
#include <string>

#include <ankerl/unordered_dense.h>

#include "ast/node.hpp"

#include "module/error.hpp"
#include "module/source_loader.hpp"

#include "syntax/parser.hpp"

#include "memory.hpp"
#include "option.hpp"
#include "result.hpp"

namespace porpoise::mod {

enum class ModuleState : u8 {
    PARSED,
    SYMBOLS_COLLECTED,
    SYMBOLS_RESOLVED,
    TYPE_CHECKED,
    ERRORED,
};

struct Module {
    std::filesystem::path path;
    std::string           source;
    ast::AST              tree;
    usize                 root_table_idx;
    ModuleState           state;

    syntax::ParserDiagnostics parse_diagnostics;
};

class ModuleManager {
  public:
    explicit ModuleManager(SourceLoader& loader) noexcept : loader_{loader} {}

    // Attempts to load the path from the loader and parse its contents
    [[nodiscard]] auto try_get(const std::filesystem::path& path)
        -> Result<opt::NonNull<Module>, Diagnostic>;

  private:
    SourceLoader&                                               loader_;
    ankerl::unordered_dense::map<std::string, mem::Box<Module>> modules_;
};

} // namespace porpoise::mod
