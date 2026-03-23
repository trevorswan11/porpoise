#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "helpers/common.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/statements/block.hpp"
#include "ast/statements/expression.hpp"

#include "syntax/keywords.hpp"

namespace porpoise::tests::helpers {

// Thin wrapper around Node is/as pattern with test assertion.
template <ast::LeafNode To, typename From> auto try_into(const From& node) -> const To& {
    CHECK(node.template is<To>());
    return ast::Node::as<To>(node);
}

template <typename N>
auto into_expression_statement(const N& node) -> const ast::ExpressionStatement& {
    return try_into<ast::ExpressionStatement>(node);
}

// Tests a syntactically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, syntax::ParserDiagnostic> && ...)
auto test_parser_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    syntax::Parser p{failing};
    auto [ast, errors] = p.consume();
    CHECK(ast.empty());

    std::array expected_arr{std::forward<Ds>(expected_diagnostics)...};
    const auto expected_count = sizeof...(Ds);

    if (errors.size() != expected_count) {
        for (const auto& e : errors) { fmt::println("{}", e); }
        CHECK(errors.size() == expected_count);
    }
    CHECK(std::ranges::equal(errors, expected_arr));
}

template <ast::LeafNode N> auto test_stmt(std::string_view input, const N& expected) -> void {
    syntax::Parser p{input};
    auto [ast, errors] = p.consume();

    check_errors<syntax::ParserDiagnostic>(errors);
    CHECK(ast.size() == 1);

    const auto  actual{std::move(ast[0])};
    const auto& actual_stmt = helpers::try_into<N>(*actual);
    CHECK(expected == actual_stmt);
}

template <ast::LeafNode N>
auto expr_stmt_from(const syntax::Token& start_token, N&& expected) -> ast::ExpressionStatement {
    return ast::ExpressionStatement{start_token, mem::make_box<N>(std::move(expected))};
}

template <ast::LeafNode N> auto make_expr_stmt(N&& expected) -> mem::Box<ast::ExpressionStatement> {
    return mem::make_box<ast::ExpressionStatement>(
        expr_stmt_from(expected.get_token(), std::move(expected)));
}

template <ast::LeafNode N>
auto test_expr_stmt(std::string_view input, const syntax::Token& start_token, N&& expected)
    -> void {
    test_stmt(input, expr_stmt_from(start_token, std::move(expected)));
}

template <ast::LeafNode N> auto test_expr_stmt(std::string_view input, N&& expected) -> void {
    test_expr_stmt(input, expected.get_token(), std::move(expected));
}

template <typename T, typename... Ts> auto make_vector(Ts&&... es) -> std::vector<T> {
    std::vector<T> list;
    list.reserve(sizeof...(es));
    (list.emplace_back(std::forward<Ts>(es)), ...);
    return list;
}

inline auto ident_from(std::string_view name) -> ast::IdentifierExpression {
    const auto extract = [](const auto& key) { return key.second; };
    const auto tt      = syntax::get_keyword(name).transform(extract).value_or(
        syntax::get_builtin(name).transform(extract).value_or(syntax::TokenType::IDENT));
    return ast::IdentifierExpression{syntax::Token{tt, name}};
}

inline auto ident_from(const syntax::Token& tok) -> ast::IdentifierExpression {
    return ast::IdentifierExpression{tok};
}

inline auto make_ident(std::string_view name) -> mem::Box<ast::IdentifierExpression> {
    return mem::make_box<ast::IdentifierExpression>(ident_from(name));
}

inline auto make_ident(const syntax::Token& tok) -> mem::Box<ast::IdentifierExpression> {
    return mem::make_box<ast::IdentifierExpression>(ident_from(tok));
}

// Creates a block statement by boxing all passed statements
template <ast::LeafNode... Ns> auto block_stmt_from(Ns&&... nodes) -> ast::BlockStatement {
    return ast::BlockStatement{
        syntax::Token{syntax::TokenType::LBRACE, "{"},
        make_vector<mem::Box<ast::Statement>>(mem::make_box<Ns>(std::forward<Ns>(nodes))...)};
}

// Creates a boxed block statement by boxing all passed statements
template <ast::LeafNode... Ns>
auto make_block_stmt(Ns&&... nodes) -> mem::Box<ast::BlockStatement> {
    return mem::make_box<ast::BlockStatement>(block_stmt_from(std::forward<Ns>(nodes)...));
}

// Creates a block statement by packing all passed expressions into expression statements
template <ast::LeafNode... Ns> auto expr_block_stmt_from(Ns&&... nodes) -> ast::BlockStatement {
    return block_stmt_from(
        ast::ExpressionStatement{nodes.get_token(), mem::make_box<Ns>(std::forward<Ns>(nodes))}...);
}

// Creates a boxed block statement by packing all passed expressions into expression statements
template <ast::LeafNode... Ns>
auto make_expr_block_stmt(Ns&&... nodes) -> mem::Box<ast::BlockStatement> {
    return mem::make_box<ast::BlockStatement>(expr_block_stmt_from(std::forward<Ns>(nodes)...));
}

template <typename... Ps>
    requires(std::same_as<Ps, ast::FunctionParameter> && ...)
auto make_parameters(Ps&&... params) -> std::vector<ast::FunctionParameter> {
    return make_vector<ast::FunctionParameter>(std::forward<Ps>(params)...);
}

namespace type_modifiers {

const ast::TypeModifier BASE{};
const ast::TypeModifier REF{ast::TypeModifier::Modifier::REF};
const ast::TypeModifier MUT_REF{ast::TypeModifier::Modifier::MUT_REF};
const ast::TypeModifier PTR{ast::TypeModifier::Modifier::PTR};
const ast::TypeModifier MUT_PTR{ast::TypeModifier::Modifier::MUT_PTR};

} // namespace type_modifiers

} // namespace porpoise::tests::helpers
