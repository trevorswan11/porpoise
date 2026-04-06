#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;
namespace mods     = helpers::type_modifiers;

TEST_CASE("Correct union") {
    helpers::test_expr_stmt(
        "union { a: i32, b: &mut T, };",
        ast::UnionExpression{
            syntax::Token{keywords::UNION},
            helpers::make_vector<ast::UnionField>(
                ast::UnionField{helpers::make_ident("a"),
                                ast::ExplicitType{mods::BASE, helpers::make_ident("i32")}},
                ast::UnionField{helpers::make_ident("b"),
                                ast::ExplicitType{mods::MUT_REF, helpers::make_ident("T")}})});
}

TEST_CASE("Illegal union field name") {
    helpers::test_parser_fail(
        "union { 2: i32 };",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found INT_10", syntax::ParserError::UNEXPECTED_TOKEN, 1, 9});
}

TEST_CASE("Illegal union field type") {
    helpers::test_parser_fail(
        "union { a: 2 };",
        syntax::ParserDiagnostic{"No prefix parse function for COLON(:) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 10uz}});
}

TEST_CASE("Empty union") {
    helpers::test_parser_fail("union { };",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_UNION, 1, 1});
}

} // namespace porpoise::tests
