#pragma once

#include <vector>

#include "diagnostic/diagnostic.hpp"

#include "iterator.hpp"
#include "option.hpp"
#include "utility.hpp"

namespace porpoise {

template <DiagnosticType D> class DiagnosticList {
  public:
    MAKE_ITERATOR(Diagnostics, std::vector<D>, diagnostics_)

  public:
    explicit DiagnosticList(opt::Option<std::string> source_path = opt::none)
        : source_path_{std::move(source_path)} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(DiagnosticList)

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

  private:
    opt::Option<std::string> source_path_;
    Diagnostics              diagnostics_;
};

} // namespace porpoise
