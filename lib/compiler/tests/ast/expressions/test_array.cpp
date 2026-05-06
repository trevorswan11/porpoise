#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

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
        "[1uz]i32{2};",
        ast::ArrayExpression{
            rbracket,
            helpers::make_primitive<ast::USizeExpression, true>("1uz"),
            false,
            ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
            helpers::make_items(helpers::primitive_from<ast::I32Expression>("2"))});

    helpers::test_expr_stmt(
        "[2uz]i32{A, B, };",
        ast::ArrayExpression{
            rbracket,
            helpers::make_primitive<ast::USizeExpression, true>("2uz"),
            false,
            ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
            helpers::make_items(helpers::ident_from("A"), helpers::ident_from("B"))});
}

TEST_CASE("Implicitly sized array") {
    helpers::test_expr_stmt(
        "[_:0]*N{a, b, c, d, e, };",
        ast::ArrayExpression{rbracket,
                             {},
                             true,
                             ast::ExplicitType{mods::PTR, helpers::make_ident("N")},
                             helpers::make_items(helpers::ident_from("a"),
                                                 helpers::ident_from("b"),
                                                 helpers::ident_from("c"),
                                                 helpers::ident_from("d"),
                                                 helpers::ident_from("e"))});
}

TEST_CASE("Array size token requirement") {
    helpers::test_parser_fail("[]i32{2};",
                              syntax::Diagnostic{syntax::Error::MISSING_ARRAY_SIZE_TOKEN, 0, 0});
}

} // namespace porpoise::tests
