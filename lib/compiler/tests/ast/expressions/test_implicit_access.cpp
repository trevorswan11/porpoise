#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace operators = syntax::operators;

TEST_CASE("Implicit access expression") {
    helpers::test_prefix_expr<ast::ImplicitAccessExpression>(operators::DOT);
}

} // namespace porpoise::tests
