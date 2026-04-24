#pragma once

#include "source_loader.hpp"

namespace porpoise::mod {

class FileLoader : public SourceLoader {
  public:
    // Attempts to obtain the file's source code from disk and load it into memory
    [[nodiscard]] auto load(const std::filesystem::path& path)
        -> Result<std::string, Error> override;
};

} // namespace porpoise::mod
