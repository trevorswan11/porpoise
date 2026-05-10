#pragma once

#include <catch2/catch_test_macros.hpp>

#include "helpers/common.hh"

#include "ast/ast.hh"

#include "syntax/parser.hh"

namespace porpoise::tests::helpers {

// Tests a syntactically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, syntax::Diagnostic> && ...)
auto test_parser_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    syntax::Parser p{failing};
    ast::AST       ast;
    auto           errors = p.consume(ast);
    REQUIRE(ast.empty());
    helpers::check_errors_against<syntax::Diagnostic>(errors,
                                                      std::forward<Ds>(expected_diagnostics)...);
}

} // namespace porpoise::tests::helpers
