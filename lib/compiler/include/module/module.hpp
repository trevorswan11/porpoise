#pragma once

#include <filesystem>
#include <string>

#include <ankerl/unordered_dense.h>

#include "ast/node.hpp"

#include "module/error.hpp"
#include "module/source_loader.hpp"

#include "sema/error.hpp"

#include "syntax/parser.hpp"

#include "array.hpp"
#include "memory.hpp"
#include "option.hpp"
#include "result.hpp"

namespace porpoise::mod {

enum class ModuleState : u8 {
    PARSED,
    SYMBOLS_COLLECTED,
    SYMBOLS_VALIDATED,
    TYPE_CHECKED,
    ERRORED,
};

using DiagnosticListVariant = std::variant<syntax::ParserDiagnostics, sema::Diagnostics>;

struct Module {
    std::filesystem::path path;
    std::string           source;
    ast::AST              tree;
    array::Index          root_table_idx;
    ModuleState           state;

    DiagnosticListVariant diagnostics;

    MAKE_VARIANT_UNPACKER(parser_diagnostics,
                          syntax::ParserDiagnostics,
                          syntax::ParserDiagnostics,
                          diagnostics,
                          std::get)
    MAKE_VARIANT_UNPACKER(
        sema_diagnostics, sema::Diagnostics, sema::Diagnostics, diagnostics, std::get)
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
