#pragma once

#include <filesystem>

#include "module/error.hpp"

#include "result.hpp"

namespace porpoise::mod {

class SourceLoader {
  public:
    virtual ~SourceLoader() = default;
    [[nodiscard]] virtual auto load(const std::filesystem::path& path)
        -> Result<std::string, Error> = 0;
};

} // namespace porpoise::mod
