#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

TEST_CASE("Illegal module location") {
    helpers::test_collector_fail(
        "const a := 2; module;",
        sema::SemaDiagnostic{"Module statement must be first statement of file",
                             sema::SemaError::ILLEGAL_MODULE_STATEMENT_LOCATION,
                             1,
                             15});
}

} // namespace porpoise::tests
