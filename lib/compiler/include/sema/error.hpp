#pragma once

#include <vector>

#include "diagnostic.hpp"
#include "result.hpp"

namespace porpoise::sema {

enum class Error : u8 {
    IDENTIFIER_REDECLARATION,
    ILLEGAL_TOP_LEVEL_STATEMENT,
    ILLEGAL_CONTROL_FLOW,
    ILLEGAL_IMPORT_LOCATION,
    ILLEGAL_MODULE_STATEMENT_LOCATION,
    ILLEGAL_NON_CONST_STATEMENT,
    FUNCTION_DECLARATION_MISSING_BODY,
    REDUNDANT_CONSTEXPR,
    DUPLICATE_MODULE_STATEMENT,
    INVALID_TABLE_IDX,
    SHADOWING_DECLARATION,
    CIRCULAR_DEPENDENCY,
    ILLEGAL_TEST_LOCATION,
};

using Diagnostic  = Diagnostic<Error>;
using Diagnostics = std::vector<Diagnostic>;

template <typename... Args> auto make_sema_err(Args&&... args) -> Err<Diagnostic> {
    return make_err<Diagnostic>(std::forward<Args>(args)...);
}

} // namespace porpoise::sema
