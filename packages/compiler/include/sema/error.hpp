#pragma once

#include <vector>

#include "diagnostic.hpp"
#include "expected.hpp"

namespace porpoise::sema {

enum class SemaError {
    IDENTIFIER_REDECLARATION,
    ILLEGAL_TOP_LEVEL_STATEMENT,
    ILLEGAL_MODULE_STATEMENT_LOCATION,
};

using SemaDiagnostic = Diagnostic<SemaError>;
using Diagnostics    = std::vector<SemaDiagnostic>;

template <typename... Args>
auto make_sema_unexpected(Args&&... args) -> Unexpected<SemaDiagnostic> {
    return make_unexpected<SemaDiagnostic>(std::forward<Args>(args)...);
}

} // namespace porpoise::sema
