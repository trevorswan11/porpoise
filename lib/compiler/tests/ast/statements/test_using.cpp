#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;
namespace mods     = helpers::type_modifiers;

TEST_CASE("Well formed using statement") {
    helpers::test_stmt(
        "using T = &[0x2uz][N]*E;",
        ast::UsingStatement{
            syntax::Token{keywords::USING},
            helpers::make_ident("T"),
            ast::ExplicitType{
                mods::REF,
                ast::ExplicitArrayType{
                    helpers::make_primitive<ast::USizeExpression, true>("0x2uz"),
                    false,
                    mem::make_box<ast::ExplicitType>(
                        mods::BASE,
                        ast::ExplicitArrayType{helpers::make_ident<true>("N"),
                                               false,
                                               mem::make_box<ast::ExplicitType>(
                                                   mods::PTR, helpers::make_ident("E"))})}}});
}

TEST_CASE("User defined type alias") {
    helpers::test_stmt(
        "pub using U = union { a: enum { A } };",
        ast::UsingStatement{
            syntax::Token{keywords::PUBLIC},
            helpers::make_ident("U"),
            ast::ExplicitType{
                mods::BASE,
                mem::make_box<ast::UnionExpression>(
                    syntax::Token{keywords::UNION},
                    helpers::make_vector<ast::UnionField>(ast::UnionField{
                        helpers::make_ident("a"),
                        ast::ExplicitType{mods::BASE,
                                          mem::make_box<ast::EnumExpression>(
                                              syntax::Token{keywords::ENUM},
                                              nullptr,
                                              helpers::make_vector<ast::Enumeration>(
                                                  ast::Enumeration{helpers::make_ident("A"), {}}),
                                              helpers::make_decls())}}),
                    helpers::make_decls())}});
}

TEST_CASE("Missing alias") {
    helpers::test_parser_fail(
        "using &[0x2uz][N]*E;",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found BW_AND", syntax::ParserError::UNEXPECTED_TOKEN, 0, 6});
}

TEST_CASE("Missing type") {
    helpers::test_parser_fail(
        "using T;",
        syntax::ParserDiagnostic{
            "Expected token ASSIGN, found SEMICOLON", syntax::ParserError::UNEXPECTED_TOKEN, 0, 7});
}

TEST_CASE("Illegal identifier alias") {
    helpers::test_parser_fail(
        "using type = T;",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found TYPE_TYPE", syntax::ParserError::UNEXPECTED_TOKEN, 0, 6});
}

} // namespace porpoise::tests
