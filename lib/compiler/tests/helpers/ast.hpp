#pragma once

#include <algorithm>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "helpers/common.hpp"

#include "ast/ast.hpp"

#include "syntax/keywords.hpp"
#include "syntax/operators.hpp" // IWYU pragma: export

#include "string.hpp"

namespace porpoise::tests::helpers {

template <ast::LeafNode Node, bool Nullable, typename... Args> auto make_leaf_node(Args&&... args) {
    if constexpr (Nullable) {
        return mem::make_nullable_box<Node>(std::forward<Args>(args)...);
    } else {
        return mem::make_box<Node>(std::forward<Args>(args)...);
    }
}

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

template <ast::LeafNode N, bool Nullable = false> auto make_expr_stmt(N&& expected) {
    return make_leaf_node<ast::ExpressionStatement, Nullable>(
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

template <bool Nullable = false> auto make_ident(std::string_view name) {
    return make_leaf_node<ast::IdentifierExpression, Nullable>(ident_from(name));
}

template <bool Nullable = false> auto make_ident(const syntax::Token& tok) {
    return make_leaf_node<ast::IdentifierExpression, Nullable>(ident_from(tok));
}

// Creates a block statement by boxing all passed statements
template <ast::LeafNode... Ns> auto block_stmt_from(Ns&&... nodes) -> ast::BlockStatement {
    return ast::BlockStatement{
        syntax::Token{syntax::TokenType::LBRACE, "{"},
        make_vector<mem::Box<ast::Statement>>(mem::make_box<Ns>(std::forward<Ns>(nodes))...)};
}

// Creates a boxed block statement by boxing all passed statements
template <bool Nullable = false, ast::LeafNode... Ns> auto make_block_stmt(Ns&&... nodes) {
    return make_leaf_node<ast::BlockStatement, Nullable>(
        block_stmt_from(std::forward<Ns>(nodes)...));
}

// Creates a block statement by packing all passed expressions into expression statements
template <ast::LeafNode... Ns> auto expr_block_stmt_from(Ns&&... nodes) -> ast::BlockStatement {
    return block_stmt_from(
        ast::ExpressionStatement{nodes.get_token(), mem::make_box<Ns>(std::forward<Ns>(nodes))}...);
}

// Creates a boxed block statement by packing all passed expressions into expression statements
template <bool Nullable = false, ast::LeafNode... Ns> auto make_expr_block_stmt(Ns&&... nodes) {
    return make_leaf_node<ast::BlockStatement, Nullable>(
        expr_block_stmt_from(std::forward<Ns>(nodes)...));
}

template <typename... Ps>
    requires(std::same_as<Ps, ast::FunctionParameter> && ...)
auto make_parameters(Ps&&... params) -> std::vector<ast::FunctionParameter> {
    return make_vector<ast::FunctionParameter>(std::forward<Ps>(params)...);
}

template <typename... Ds>
    requires(std::same_as<Ds, ast::DeclStatement> && ...)
auto make_decls(Ds&&... decls) -> std::vector<mem::Box<ast::DeclStatement>> {
    return make_vector<mem::Box<ast::DeclStatement>>(mem::make_box<Ds>(std::forward<Ds>(decls))...);
}

template <ast::PrimitiveNode N> auto primitive_from(std::string_view str) noexcept -> N {
    syntax::Lexer l{str};
    const auto    tok = l.advance();

    CHECK(syntax::token_type::is_number(tok.type));
    auto value = ast::parse_primitive_value<typename N::value_type, false>(str, tok.type);
    CHECK(value);
    return N{syntax::Token{tok.type, str}, *value};
}

// For strings:include  surrounding quotes in the input if needed, multiline strings not supported
template <ast::PrimitiveNode Primitive, bool Nullable = false>
auto make_primitive(std::string_view str) noexcept {
    if constexpr (std::is_same_v<Primitive, ast::StringExpression>) {
        const auto trimmed = string::trim(str, [](byte b) { return b == '"'; });
        return make_leaf_node<ast::StringExpression, Nullable>(
            syntax::Token{syntax::TokenType::STRING, str}, std::string{trimmed});
    } else {
        return make_leaf_node<Primitive, Nullable>(primitive_from<Primitive>(str));
    }
}

template <ast::PrimitiveNode N, bool Nullable = false>
    requires(std::same_as<N, ast::VoidExpression>)
auto make_primitive() noexcept {
    return make_leaf_node<ast::VoidExpression, Nullable>(
        syntax::Token{syntax::TokenType::LBRACE, "{"}, Unit{});
}

template <ast::PrimitiveNode N, bool Nullable = false>
    requires(std::same_as<N, ast::BoolExpression>)
auto make_primitive(bool value) noexcept {
    const syntax::Token tok{value ? syntax::keywords::BOOLEAN_TRUE
                                  : syntax::keywords::BOOLEAN_FALSE};
    return make_leaf_node<ast::BoolExpression, Nullable>(tok, value ? true : false);
}

namespace type_modifiers {

const ast::TypeModifier BASE{};
const ast::TypeModifier REF{ast::TypeModifier::Modifier::REF};
const ast::TypeModifier MUT_REF{ast::TypeModifier::Modifier::MUT_REF};
const ast::TypeModifier PTR{ast::TypeModifier::Modifier::PTR};
const ast::TypeModifier MUT_PTR{ast::TypeModifier::Modifier::MUT_PTR};

} // namespace type_modifiers

} // namespace porpoise::tests::helpers
