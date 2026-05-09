#pragma once

#include <ostream>

#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "module/module.hh"

namespace porpoise::sema {

// A contextual wrapper around sematic steps
//
// Owns its own diagnostic list
struct Context {
    mod::ModuleManager&  modules;
    SymbolTableRegistry& registry;
    TypePool&            pool;
    Diagnostics          diagnostics;
    std::ostream&        error_stream;
    opt::Index           prelude_index;

    Context(mod::ModuleManager&  m,
            SymbolTableRegistry& r,
            TypePool&            p,
            Diagnostics          d,
            std::ostream&        s) noexcept
        : modules{m}, registry{r}, pool{p}, diagnostics{std::move(d)}, error_stream{s} {}
    ~Context() = default;

    // Creates a copy with identical data but a new diagnostic list
    Context(const Context& other)
        : modules{other.modules}, registry{other.registry}, pool{other.pool},
          diagnostics{other.diagnostics.create_new()}, error_stream{other.error_stream},
          prelude_index{other.prelude_index} {}

    // Creates a copy with identical data but a new diagnostic list
    auto operator=(const Context& other) -> Context& {
        diagnostics = other.diagnostics.create_new();
        return *this;
    }

    Context(Context&&) noexcept           = default;
    auto operator=(Context&&) -> Context& = delete;

    // Returns false if the passed result was an error type, which is forwarded to the diagnostics
    template <typename T = Unit> auto try_result(Result<T, Diagnostic>&& result) -> bool {
        if (!result) {
            diagnostics.emplace_back(result.error());
            return false;
        }
        return true;
    }

    // Gets the already-resolved poison type from the pool
    [[nodiscard]] auto get_poison() -> Type&;

    // Poisons the symbol and constructs an associated diagnostic to insert into the list
    template <typename... Args> auto poison_symbol(Symbol& symbol, Args&&... args) -> void {
        if constexpr (sizeof...(args) != 0) {
            diagnostics.emplace_back(std::forward<Args>(args)...);
        }
        symbol.set_type(get_poison());
        symbol.set_status(ResolveStatus::RESOLVED);
    }

    // Poisons the node and constructs an associated diagnostic to insert into the list
    template <typename... Args>
    auto poison_node(mod::Module& module, const ast::NodeID& id, Args&&... args) -> void {
        if constexpr (sizeof...(args) != 0) {
            diagnostics.emplace_back(std::forward<Args>(args)...);
        }
        module.tree.set_sema_type(id, get_poison());
    }

    // Creates and injects the builtin/primitive prelude and sets the internal prelude index
    auto inject_prelude() -> void;
};

} // namespace porpoise::sema
