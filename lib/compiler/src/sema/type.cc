#include "sema/type.hh"

#include "option.hh"

namespace porpoise::sema {

auto TypePool::get_opt(const types::Key& key) noexcept -> opt::Option<Type&> {
    if (auto it = cache_.find(key); it != cache_.end()) { return *it->second; }
    return opt::none;
}

auto TypePool::get_or_emplace(const types::Key& key) -> Type& {
    if (auto type = get_opt(key)) { return *type; }
    auto* type = arena_.make<Type>(key).get();
    cache_.emplace(key, type);
    return *type;
}

namespace {

auto strip_modifiers(TypePool& pool, const Type& old_type, types::MutabilityModifiers mut)
    -> Type& {
    auto key = old_type.get_key();
    key.set_mut(key.get_mut() & ~mut);

    // Resolve here since the type information doesn't contain modifier information
    auto& new_type = pool[key];
    if (!new_type.has_resolved()) { new_type.resolve(old_type.get_resolved()); }
    return new_type;
}

} // namespace

auto TypePool::strip_const(const Type& type) -> Type& {
    if (!type.is_constant()) { return const_cast<Type&>(type); }
    return strip_modifiers(*this, type, types::mut::CONSTANT);
}

auto TypePool::strip_volatile(const Type& type) -> Type& {
    if (!type.is_volatile()) { return const_cast<Type&>(type); }
    return strip_modifiers(*this, type, types::mut::VOLATILE);
}

} // namespace porpoise::sema
