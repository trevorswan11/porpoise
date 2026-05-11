#include "sema/type.hh"

namespace porpoise::sema {

auto TypePool::get_opt(const types::Key& key) noexcept -> opt::Option<Type&> {
    if (auto it = cache_.find(key); it != cache_.end()) { return *it->second; }
    return opt::none;
}

auto TypePool::get_or_emplace(const types::Key& key) -> Type& {
    if (auto type = get_opt(key)) { return *type; }
    auto* type = arena_.make<Type>(key.get_kind()).get();
    cache_.emplace(key, type);
    return *type;
}

} // namespace porpoise::sema
