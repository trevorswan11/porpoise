#pragma once

#include "sema/error.hpp"
#include "sema/module/module.hpp"
#include "sema/pool.hpp"
#include "sema/symbol.hpp"

namespace porpoise::sema {

// A contextual wrapper around sematic steps
struct Context {
    mod::ModuleManager&  modules;
    SymbolTableRegistry& registry;
    TypePool&            pool;
    Diagnostics&         diagnostics;

    // Creates a new context with a new diagnostics pointer
    [[nodiscard]] auto copy(Diagnostics& new_diags) const noexcept -> Context {
        return {modules, registry, pool, new_diags};
    }

    // Returns false if the passed result was an error type, which is forwarded to the diagnostics
    template <typename T = Unit> auto try_result(Result<T, Diagnostic>&& result) -> bool {
        if (!result) {
            diagnostics.emplace_back(result.error());
            return false;
        }
        return true;
    }
};

} // namespace porpoise::sema
