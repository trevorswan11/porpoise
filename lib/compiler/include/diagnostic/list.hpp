#pragma once

#include <ostream>
#include <span>
#include <vector>

#include <fmt/ostream.h>

#include "diagnostic/diagnostic.hpp"

#include "iterator.hpp"
#include "option.hpp"
#include "utility.hpp"

namespace porpoise {

namespace sema::mod { struct Module; } // namespace sema::mod

namespace detail {

// Formats the diagnostic with the modules information, falling back to standard formatting
auto format_module_diagnostic(std::ostream&                         os,
                              FormattableDiagnostic&&               diag,
                              opt::Option<const sema::mod::Module&> module) -> std::ostream&;

} // namespace detail

// Stored diagnostics are technically module-agnostic
template <DiagnosticType D> class DiagnosticList {
  public:
    MAKE_ITERATOR(Diagnostics, std::vector<D>, diagnostics_)

  public:
    DiagnosticList() noexcept = default;
    ~DiagnosticList()         = default;

    MAKE_MOVE_ONLY(DiagnosticList)

    auto push_back(const D& d) -> void { diagnostics_.push_back(d); }

    template <typename... Args> auto emplace_back(Args&&... args) -> void {
        diagnostics_.emplace_back(std::forward<Args>(args)...);
    }

    template <typename Self>
    [[nodiscard]] auto operator[](this Self&& self, usize idx) noexcept -> auto& {
        return self.diagnostics_[idx];
    }

    template <typename Self> [[nodiscard]] auto at(this Self&& self, usize idx) -> auto& {
        return self.diagnostics_.at(idx);
    }

    operator std::span<const D>() const { return diagnostics_; }

    // Prints the diagnostics with information from the enclosing module if provided
    auto print(std::ostream& os, opt::Option<const sema::mod::Module&> module = {}) const -> void {
        for (const auto& diag : diagnostics_) {
            detail::format_module_diagnostic(os, diag.to_formattable(), module) << "\n";
        }
    }

  private:
    Diagnostics diagnostics_;
};

} // namespace porpoise
