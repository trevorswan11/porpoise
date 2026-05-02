#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "module/module.hpp"

#include "sema/pool.hpp"
#include "sema/symbol.hpp"

namespace porpoise::sema {

// The manager for all steps of semantic analysis.
class Analyzer {
  public:
    explicit Analyzer(mod::ModuleManager& modules,
                      std::ostream&       error_stream,
                      opt::Option<bool>   in_terminal) noexcept
        : modules_{modules}, error_stream_{error_stream}, in_terminal_{in_terminal} {}
    ~Analyzer() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Analyzer)

    // Runs the entire sema pipeline
    auto analyze(const std::filesystem::path& entry_path) -> Result<Unit, Diagnostic>;

    template <typename Self> [[nodiscard]] auto get_table(this Self&& self, usize idx) -> auto& {
        return self.registry_.get(idx);
    }

    template <typename Self>
    [[nodiscard]] auto get_table_opt(this Self&& self, usize idx) noexcept -> auto& {
        return self.registry_.get_opt(idx);
    }

    MAKE_DEDUCING_GETTER(registry, SymbolTableRegistry&)
    MAKE_DEDUCING_GETTER(pool, TypePool&)

    auto collect_symbols(mod::Module& module) -> void;
    auto resolve_types(mod::Module& module) -> void;

  private:
    mod::ModuleManager& modules_;
    SymbolTableRegistry registry_;
    TypePool            pool_;
    std::ostream&       error_stream_;
    opt::Option<bool>   in_terminal_;

    std::vector<std::string> collection_stack_;
};

} // namespace porpoise::sema
