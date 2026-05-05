#include "sema/context.hpp"

namespace porpoise::sema {

auto Context::get_poison() -> Type& {
    auto& poison = pool[{TypeKind::POISON, false}];
    if (!poison.has_resolved()) { poison.resolve<types::Poison>(); }
    return poison;
}

auto Context::make_type_prelude() -> void {
    // This should only ever do work once
    if (prelude_index) { return; }
    const auto prelude = registry.create();

    prelude_index.emplace(prelude);
}

} // namespace porpoise::sema
