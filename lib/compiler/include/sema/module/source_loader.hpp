#pragma once

#include <filesystem>

#include "sema/error.hpp"

#include "result.hpp"

namespace porpoise::sema::mod {

class SourceLoader {
  public:
    virtual ~SourceLoader() = default;
    [[nodiscard]] virtual auto load(const std::filesystem::path& path)
        -> Result<std::string, Diagnostic> = 0;

    // Normalizes the path to behave as required by the loader
    [[nodiscard]] virtual auto normalize(const std::filesystem::path& path)
        -> Result<std::filesystem::path, Error> = 0;
};

} // namespace porpoise::sema::mod
