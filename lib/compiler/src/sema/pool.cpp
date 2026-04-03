#include "sema/pool.hpp"

namespace porpoise::sema {

auto TypePool::get(const types::Key& key) noexcept -> Optional<Type&> {
    if (auto it = cache_.find(key); it != cache_.end()) { return *it->second; }
    return std::nullopt;
}

auto TypePool::get_or_emplace(const types::Key& key) -> Type& {
    if (auto type = get(key)) { return *type; }
    auto* type = arena_.make<Type>(key.get_kind()).get();
    cache_.insert({key, type});
    return *type;
}

} // namespace porpoise::sema
