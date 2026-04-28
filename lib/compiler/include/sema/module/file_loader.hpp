#pragma once

#include "source_loader.hpp"

namespace porpoise::sema::mod {

class FileLoader : public SourceLoader {
  public:
    // Attempts to obtain the file's source code from disk and load it into memory
    [[nodiscard]] auto load(const std::filesystem::path& path)
        -> Result<std::string, Diagnostic> override;

    // Converts the path to its weakly canonical representation
    [[nodiscard]] auto normalize(const std::filesystem::path& path)
        -> Result<std::filesystem::path, Error> override;
};

} // namespace porpoise::sema::mod
