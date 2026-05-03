#pragma once

#include "diagnostic.hpp"
#include "result.hpp"

namespace porpoise::mod {

enum class Error : u8 {
    PATH_DOES_NOT_EXIST,
    PATH_IS_NOT_FILE,
    FAILED_TO_OPEN_FILE,
    NORMALIZATION_FAILED,
    MODULE_PATH_NOT_RELATIVE,
    MODULE_DOES_NOT_EXIST,
    MODULE_ALREADY_EXISTS,
};

using Diagnostic = Diagnostic<Error>;

template <typename... Args> auto make_mod_err(Args&&... args) -> Err<Diagnostic> {
    return make_err<Diagnostic>(std::forward<Args>(args)...);
}

} // namespace porpoise::mod
