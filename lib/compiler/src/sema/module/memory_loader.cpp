#include "sema/module/memory_loader.hpp"

namespace porpoise::sema::mod {

auto MemoryLoader::add(std::filesystem::path path, const std::string& content) -> void {
    auto normalized = normalize(path);
    assert(normalized);
    files_[*normalized] = std::move(content);
}

auto MemoryLoader::load(const std::filesystem::path& path) -> Result<std::string, Error> {
    auto normalized = normalize(path);
    assert(normalized);
    auto it = files_.find(normalized->string());
    if (it == files_.end()) { return Err{Error::PATH_DOES_NOT_EXIST}; }
    return it->second;
}

} // namespace porpoise::sema::mod
