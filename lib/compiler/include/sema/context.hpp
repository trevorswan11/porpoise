#pragma once

#include <ostream>

#include "sema/error.hpp"
#include "sema/pool.hpp"
#include "sema/symbol.hpp"

#include "module/module.hpp"

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

    // Creates and injects the builtin/primitive prelude and sets the internal prelude index
    auto make_type_prelude() -> void;
};

} // namespace porpoise::sema
