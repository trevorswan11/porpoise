#pragma once

#include <filesystem>

#include "sema/error.hpp"

#include "result.hpp"

namespace porpoise::sema::mod {

class SourceLoader {
  public:
    virtual ~SourceLoader() = default;
    [[nodiscard]] virtual auto load(const std::filesystem::path& path)
        -> Result<std::string, Error> = 0;

    // Normalizes the path to behave as required by the implementation
    [[nodiscard]] virtual auto normalize(const std::filesystem::path& path)
        -> Result<std::filesystem::path, Error> {
        return path;
    }
};

} // namespace porpoise::sema::mod
