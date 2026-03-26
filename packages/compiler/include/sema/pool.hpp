#pragma once

#include <ankerl/unordered_dense.h>

#include "sema/type.hpp"

#include "arena.hpp"

namespace porpoise::sema {

// All associated type lifetimes are tied to the pool
class TypePool {
  public:
    TypePool() noexcept = default;
    ~TypePool()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(TypePool)

    // Gets a type by its key or emplace's it into the internal cache
    [[nodiscard]] auto get_or_emplace(const types::Key& key) -> Type&;
    [[nodiscard]] auto get(const types::Key& key) -> Optional<Type&>;

  private:
    mem::Arena                                      arena_;
    ankerl::unordered_dense::map<types::Key, Type*> cache_;
};

} // namespace porpoise::sema
