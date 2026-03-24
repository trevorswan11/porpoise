#pragma once

#include "arena.hpp"

namespace porpoise::sema {

class TypePool {
  public:
    TypePool() noexcept  = default;
    ~TypePool() noexcept = default;

    TypePool(const TypePool&)                    = delete;
    auto operator=(const TypePool&) -> TypePool& = delete;
    TypePool(TypePool&& other) noexcept          = default;
    auto operator=(TypePool&&) -> TypePool&      = delete;

  private:
    mem::Arena arena_;
};

} // namespace porpoise::sema
