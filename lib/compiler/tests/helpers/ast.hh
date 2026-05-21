#pragma once

#include <concepts>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "ast/ast.hh"
#include "helpers/common.hh"
#include "syntax/error.hh"
#include "syntax/parser.hh"

namespace porpoise::tests::helpers {

template <typename D>
concept SyntaxDiag = std::same_as<D, syntax::Diagnostic>;

// Tests a syntactically failing input against the expected generated errors
template <SyntaxDiag... Ds>
auto test_parser_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    syntax::Parser p{failing};
    ast::AST       ast;
    auto           errors = p.consume(ast);
    REQUIRE(ast.empty());
    helpers::check_errors_against<syntax::Diagnostic>(errors,
                                                      std::forward<Ds>(expected_diagnostics)...);
}

} // namespace porpoise::tests::helpers
