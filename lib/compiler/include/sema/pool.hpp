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
    [[nodiscard]] auto operator[](const types::Key& key) -> Type& { return get_or_emplace(key); }
    [[nodiscard]] auto get(const types::Key& key) noexcept -> Optional<Type&>;
    [[nodiscard]] auto builtin(TypeKind kind) noexcept -> Type& {
        return get_or_emplace({kind, false, 0});
    }

  private:
    auto get_or_emplace(const types::Key& key) -> Type&;

  private:
    mem::Arena                                      arena_;
    ankerl::unordered_dense::map<types::Key, Type*> cache_;
};

} // namespace porpoise::sema
