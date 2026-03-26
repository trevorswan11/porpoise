#pragma once

#include <vector>

#include "diagnostic.hpp"
#include "expected.hpp"

namespace porpoise::sema {

enum class Error : u8 {
    IDENTIFIER_REDECLARATION,
    ILLEGAL_TOP_LEVEL_STATEMENT,
    ILLEGAL_IMPORT_LOCATION,
    ILLEGAL_MODULE_STATEMENT_LOCATION,
    DUPLICATE_MODULE_STATEMENT,
    INVALID_TABLE_IDX,
    SHADOWING_DECLARATION,
};

using Diagnostic  = Diagnostic<Error>;
using Diagnostics = std::vector<Diagnostic>;

template <typename... Args> auto make_sema_unexpected(Args&&... args) -> Unexpected<Diagnostic> {
    return make_unexpected<Diagnostic>(std::forward<Args>(args)...);
}

} // namespace porpoise::sema
