#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/array.hpp"
#include "ast/expressions/function.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/type.hpp"

namespace porpoise::tests {

using Items = std::vector<mem::Box<ast::Expression>>;

namespace helpers {

template <ast::LeafNode... Ns> auto make_items(Ns&&... nodes) -> Items {
    return make_vector<mem::Box<ast::Expression>>(mem::make_box<Ns>(std::forward<Ns>(nodes))...);
}

} // namespace helpers

namespace mods = helpers::type_modifiers;

const syntax::Token rbracket{syntax::TokenType::LBRACKET, "["};

TEST_CASE("Explicitly sized arrays") {
    helpers::test_expr_stmt(
        "[1uz]int{2};",
        ast::ArrayExpression{rbracket,
                             mem::make_box<ast::USizeIntegerExpression>(
                                 syntax::Token{syntax::TokenType::UZINT_10, "1uz"}, 1),
                             ast::ExplicitType{mods::BASE, helpers::make_ident("int")},
                             helpers::make_items(ast::SignedIntegerExpression{
                                 syntax::Token{syntax::TokenType::INT_10, "2"}, 2})});

    helpers::test_expr_stmt(
        "[2uz]int{A, B, };",
        ast::ArrayExpression{
            rbracket,
            mem::make_box<ast::USizeIntegerExpression>(
                syntax::Token{syntax::TokenType::UZINT_10, "2uz"}, 2),
            ast::ExplicitType{mods::BASE, helpers::make_ident("int")},
            helpers::make_items(helpers::ident_from("A"), helpers::ident_from("B"))});
}

TEST_CASE("Implicitly sized array") {
    helpers::test_expr_stmt(
        "[_]*N{a, b, c, d, e, };",
        ast::ArrayExpression{rbracket,
                             std::nullopt,
                             ast::ExplicitType{mods::PTR, helpers::make_ident("N")},
                             helpers::make_items(helpers::ident_from("a"),
                                                 helpers::ident_from("b"),
                                                 helpers::ident_from("c"),
                                                 helpers::ident_from("d"),
                                                 helpers::ident_from("e"))});
}

TEST_CASE("Size mismatch") {
    helpers::test_parser_fail(
        "[1uz]int{2, 3};",
        syntax::ParserDiagnostic{syntax::ParserError::EXPLICIT_ARRAY_SIZE_MISMATCH, 1, 2});
}

TEST_CASE("Array size token requirement") {
    helpers::test_parser_fail(
        "[]int{2};", syntax::ParserDiagnostic{syntax::ParserError::MISSING_ARRAY_SIZE_TOKEN, 1, 1});
    helpers::test_parser_fail(
        "[3]int{1,2,3};",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_ARRAY_SIZE_TYPE, 1, 2});
    helpers::test_parser_fail(
        R"(["e"]int{1};)",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_ARRAY_SIZE_TYPE, 1, 2});
}

} // namespace porpoise::tests
