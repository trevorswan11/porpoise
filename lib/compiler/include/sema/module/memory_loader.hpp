#pragma once

#include <string>

#include <ankerl/unordered_dense.h>

#include "sema/module/source_loader.hpp"

namespace porpoise::sema::mod {

// A mock loader used for in-memory files that can't be referenced relatively
class MemoryLoader : public SourceLoader {
  public:
    // Add a file to the virtual file system. Allows overwriting
    auto add(std::filesystem::path path, const std::string& content) -> void;

    [[nodiscard]] auto load(const std::filesystem::path& path)
        -> Result<std::string, Error> override;

    auto normalize(const std::filesystem::path& path)
        -> Result<std::filesystem::path, Error> override {
        return path.lexically_normal();
    }

  private:
    ankerl::unordered_dense::map<std::filesystem::path, std::string> files_;
};

} // namespace porpoise::sema::mod
