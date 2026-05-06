#pragma once

#include <filesystem>
#include <ostream>
#include <string>

#include <ankerl/unordered_dense.h>

#include "ast/node.hpp"

#include "sema/error.hpp"

#include "syntax/error.hpp"

#include "module/source_loader.hpp"

#include "memory.hpp"
#include "result.hpp"
#include "source_file.hpp"
#include "utility.hpp"
#include "variant.hpp"

namespace porpoise::mod {

enum class ModuleState : u8 {
    PARSED,
    SYMBOLS_COLLECTED,
    TYPE_RESOLVED,
    TYPE_CHECKED,

    POISONED_SYMBOL_COLLECTION,
    POISONED_TYPE_RESOLVED,
    POISONED_TYPE_CHECKED,
    ERRORED,
};

// cppcheck-suppress-begin internalAstError
using DiagnosticListVariant = std::variant<Unit, syntax::Diagnostics, sema::Diagnostics>;
// cppcheck-suppress-end internalAstError

#define MAKE_MODULE_DIAGNOSTIC_UNPACKER(name, checker, DiagType)                \
    [[nodiscard]] auto CONCAT(get_, name)() const noexcept -> const DiagType& { \
        try {                                                                   \
            return std::get<DiagType>(diagnostics);                             \
        } catch (...) { std::unreachable(); }                                   \
    }                                                                           \
                                                                                \
    [[nodiscard]] auto CONCAT(has_, name)() const noexcept -> bool {            \
        return checker() && std::holds_alternative<DiagType>(diagnostics);      \
    }

struct Module {
    std::filesystem::path path;
    std::filesystem::path parent_path;
    SourceFile            source;
    ast::AST              tree;
    opt::Index            root_table_idx;
    ModuleState           state;

    DiagnosticListVariant diagnostics{Unit{}};

    MAKE_MODULE_DIAGNOSTIC_UNPACKER(parser_diagnostics, is_errored, syntax::Diagnostics)
    MAKE_MODULE_DIAGNOSTIC_UNPACKER(sema_diagnostics, is_poisoned, sema::Diagnostics)

    MAKE_VARIANT_MATCHER(diagnostics)

    // Errors out the module regardless of previous state and emplaces the diagnostics
    template <typename DiagList>
        requires(!std::same_as<std::remove_cvref_t<DiagList>, Unit>)
    auto error_out(DiagList&& list, ModuleState error_state) noexcept -> mod::ModuleState {
        diagnostics.emplace<DiagList>(std::move(list));
        return state = error_state;
    }

    // Prints the modules diagnostics to the stream, doing nothing if an error state is not present
    auto print_diagnostics(std::ostream& os) const -> void;

    // Errored modules cannot be used in any future compilation step
    auto is_errored() const noexcept -> bool { return state == ModuleState::ERRORED; }

    // Poisoned modules are able to be used in sematic steps but are not correct themselves
    auto is_poisoned() const noexcept -> bool {
        return state >= ModuleState::POISONED_SYMBOL_COLLECTION && !is_errored();
    }

    // Indicates if the module is neither errored nor poisoned
    auto is_ok() const noexcept -> bool { return state < ModuleState::POISONED_SYMBOL_COLLECTION; }

    auto is_collectable() const noexcept -> bool {
        return state == mod::ModuleState::PARSED && !root_table_idx;
    }

    // Checks if symbol collection has run, allowing poisoned states
    auto is_resolvable() const noexcept -> bool {
        return root_table_idx && (state == mod::ModuleState::SYMBOLS_COLLECTED ||
                                  state == mod::ModuleState::POISONED_SYMBOL_COLLECTION);
    }
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

} // namespace porpoise::mod
