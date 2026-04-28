#pragma once

#include <filesystem>
#include <ostream>
#include <string>

#include <ankerl/unordered_dense.h>

#include "ast/node.hpp"

#include "sema/error.hpp"
#include "sema/module/source_loader.hpp"

#include "syntax/parser.hpp"

#include "diagnostic/source_file.hpp"

#include "array.hpp"
#include "memory.hpp"
#include "result.hpp"
#include "utility.hpp"

namespace porpoise::sema::mod {

enum class ModuleState : u8 {
    PARSED,
    SYMBOLS_COLLECTED,
    SYMBOLS_VALIDATED,
    TYPE_CHECKED,
    ERRORED,
};

using DiagnosticListVariant = std::variant<syntax::ParserDiagnostics, sema::Diagnostics>;

#define MAKE_MODULE_DIAGNOSTIC_UNPACKER(name, DiagType)                         \
    [[nodiscard]] auto CONCAT(get_, name)() const noexcept -> const DiagType& { \
        try {                                                                   \
            return std::get<DiagType>(diagnostics);                             \
        } catch (...) { std::unreachable(); }                                   \
    }                                                                           \
                                                                                \
    [[nodiscard]] auto CONCAT(has_, name)() const noexcept -> bool {            \
        return is_errored() && std::holds_alternative<DiagType>(diagnostics);   \
    }

struct Module {
    std::filesystem::path path;
    std::filesystem::path parent_path;
    SourceFile            source;
    ast::AST              tree;
    array::Index          root_table_idx;
    ModuleState           state;

    DiagnosticListVariant diagnostics;

    MAKE_MODULE_DIAGNOSTIC_UNPACKER(parser_diagnostics, syntax::ParserDiagnostics)
    MAKE_MODULE_DIAGNOSTIC_UNPACKER(sema_diagnostics, sema::Diagnostics)

    MAKE_VARIANT_MATCHER(diagnostics)

    // Errors out the module regardless of previous state and emplaces the diagnostics
    template <typename DiagList> auto error_out(DiagList&& list) noexcept -> mod::ModuleState {
        state = ModuleState::ERRORED;
        diagnostics.emplace<DiagList>(std::move(list));
        return state;
    }

    // Prints the modules diagnostics to the stream, doing nothing if an error state is not present
    auto print_diagnostics(std::ostream& os) const -> void;
    auto is_errored() const noexcept -> bool { return state == ModuleState::ERRORED; }
};

#undef MAKE_MODULE_DIAGNOSTIC_UNPACKER

class ModuleManager {
  public:
    explicit ModuleManager(SourceLoader& loader) noexcept : loader_{loader} {}
    ~ModuleManager() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ModuleManager)

    // Attempts to load the path from the loader and parse its contents.
    //
    // Asserts that the path is relative and its parent is absolute
    [[nodiscard]] auto try_get_file_module(const std::filesystem::path& path,
                                           const std::filesystem::path& parent_path = {})
        -> Result<mem::NonNull<Module>, Diagnostic>;

    // Attempts to load the module from the loader and parse its contents
    [[nodiscard]] auto try_get_library_module(const std::string& name)
        -> Result<mem::NonNull<Module>, Diagnostic>;

    // Adds a library module and its underlying path to the lookup table
    [[nodiscard]] auto add_library_module(const std::string&           name,
                                          const std::filesystem::path& path)
        -> Result<Unit, Diagnostic>;

  private:
    [[nodiscard]] auto try_get(const std::filesystem::path& path)
        -> Result<mem::NonNull<Module>, Diagnostic>;

  private:
    SourceLoader&                                                         loader_;
    ankerl::unordered_dense::map<std::filesystem::path, mem::Box<Module>> modules_;

    // Maps physical porpoise modules to their path on disk
    ankerl::unordered_dense::map<std::string, std::filesystem::path> module_lut_;
};

} // namespace porpoise::sema::mod
