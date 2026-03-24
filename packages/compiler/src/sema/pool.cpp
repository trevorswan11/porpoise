#include "sema/pool.hpp"

namespace porpoise::sema {

auto TypePool::get(const types::Key& key) -> Type& {
    if (auto it = cache_.find(key); it != cache_.end()) { return *it->second; }
    auto* type = arena_.make<Type>(key.get_kind()).get();
    cache_.insert({key, type});
    return *type;
}

} // namespace porpoise::sema
