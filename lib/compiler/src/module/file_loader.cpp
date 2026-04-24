#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#include "module/file_loader.hpp"

namespace porpoise::mod {

auto FileLoader::load(const std::filesystem::path& path) -> Result<std::string, Error> {
    std::error_code ec;
    const auto      canonical_path = std::filesystem::weakly_canonical(path, ec);
    if (ec) { return Err{Error::CANONICALIZATION_FAILED}; }

    if (!std::filesystem::exists(canonical_path)) {
        return Err{Error::PATH_DOES_NOT_EXIST};
    } else if (std::filesystem::is_regular_file(canonical_path)) {
        return Err{Error::PATH_IS_NOT_FILE};
    }

    std::ifstream file{canonical_path, std::ios::in | std::ios::binary};
    if (!file.is_open()) { return Err{Error::FAILED_TO_OPEN_FILE}; }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

} // namespace porpoise::mod
