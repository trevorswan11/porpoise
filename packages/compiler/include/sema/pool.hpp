#pragma once

#include <ankerl/unordered_dense.h>

#include "sema/type.hpp"

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

    // Gets a type by its key or emplace's it into the internal cache
    [[nodiscard]] auto get(const types::Key& key) -> Type&;

  private:
    mem::Arena                                      arena_;
    ankerl::unordered_dense::map<types::Key, Type*> cache_;
};

} // namespace porpoise::sema
