#pragma once

#include <filesystem>
#include <string>

#include <ankerl/unordered_dense.h>

#include "ast/node.hpp"

#include "sema/error.hpp"
#include "sema/module/source_loader.hpp"

#include "syntax/parser.hpp"

#include "array.hpp"
#include "memory.hpp"
#include "option.hpp"
#include "result.hpp"

namespace porpoise::sema::mod {

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
    MAKE_VARIANT_MATCHER(diagnostics)

    // Errors out the module regardless of previous state and emplaces the diagnostics
    template <typename DiagList> auto error_out(DiagList&& list) noexcept -> mod::ModuleState {
        state = ModuleState::ERRORED;
        diagnostics.emplace<DiagList>(std::move(list));
        return state;
    }

    auto is_errored() const noexcept { return state == ModuleState::ERRORED; }
};

class ModuleManager {
  public:
    explicit ModuleManager(SourceLoader& loader) noexcept : loader_{loader} {}

    // Attempts to load the path from the loader and parse its contents
    [[nodiscard]] auto try_get_file_module(const std::filesystem::path& path)
        -> Result<opt::NonNull<Module>, Diagnostic>;

    // Attempts to load the module from the loader and parse its contents
    [[nodiscard]] auto try_get_true_module(const std::string& name)
        -> Result<opt::NonNull<Module>, Diagnostic>;

    // Adds a true module and its underlying path to the lookup table
    [[nodiscard]] auto add_porpoise_module(const std::string&           name,
                                           const std::filesystem::path& path)
        -> Result<Unit, Diagnostic>;

  private:
    [[nodiscard]] auto try_get(const std::filesystem::path& path)
        -> Result<opt::NonNull<Module>, Diagnostic>;

  private:
    SourceLoader&                                                         loader_;
    ankerl::unordered_dense::map<std::filesystem::path, mem::Box<Module>> modules_;

    // Maps physical porpoise modules to their path on disk
    ankerl::unordered_dense::map<std::string, std::filesystem::path> module_lut_;
};

} // namespace porpoise::sema::mod
