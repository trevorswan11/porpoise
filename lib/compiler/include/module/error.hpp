#pragma once

#include "diagnostic/diagnostic.hpp"

#include "result.hpp"
#include "types.hpp"

namespace porpoise::mod {

enum class Error : u8 {
    CANONICALIZATION_FAILED,
    PATH_DOES_NOT_EXIST,
    PATH_IS_NOT_FILE,
    FAILED_TO_OPEN_FILE,
};

using Diagnostic = Diagnostic<Error>;

template <typename... Args> auto make_module_err(Args&&... args) -> Err<Diagnostic> {
    return make_err<Diagnostic>(std::forward<Args>(args)...);
}

} // namespace porpoise::mod
