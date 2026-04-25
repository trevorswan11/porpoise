#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#include "sema/module/file_loader.hpp"

namespace porpoise::sema::mod {

auto FileLoader::load(const std::filesystem::path& path) -> Result<std::string, Error> {
    if (!std::filesystem::exists(path)) {
        return Err{Error::PATH_DOES_NOT_EXIST};
    } else if (std::filesystem::is_regular_file(path)) {
        return Err{Error::PATH_IS_NOT_FILE};
    }

    std::ifstream file{path, std::ios::in | std::ios::binary};
    if (!file.is_open()) { return Err{Error::FAILED_TO_OPEN_FILE}; }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

auto FileLoader::normalize(const std::filesystem::path& path)
    -> Result<std::filesystem::path, Error> {
    std::error_code ec;
    const auto      canonical_path = std::filesystem::weakly_canonical(path, ec);
    if (ec) { return Err{Error::NORMALIZATION_FAILED}; }
    return canonical_path;
}

} // namespace porpoise::sema::mod
