#include <algorithm>
#include <ranges>
#include <utility>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "array.hpp"

#include "syntax/keywords.hpp"
#include "syntax/parser.hpp"
#include "syntax/precedence.hpp"
#include "syntax/token.hpp"

#include "ast/ast.hpp"

namespace porpoise::syntax {

syntax::Parser::Checkpoint::Checkpoint(const syntax::Parser& parser) noexcept
    : snapshot_{parser.lexer_}, current_{parser.current_token_}, peek_{parser.peek_token_} {}

auto syntax::Parser::reset(std::string_view input) noexcept -> void {
    *this = syntax::Parser{input};
}

auto syntax::Parser::advance(u8 times) noexcept -> const syntax::Token& {
    for (u8 i = 0; i < times; ++i) {
        if (current_token_.type == syntax::TokenType::END &&
            (input_.empty() && current_token_.is_at_start())) {
            break;
        }

        current_token_ = peek_token_;
        peek_token_    = lexer_.advance();
    }
    return current_token_;
}

auto syntax::Parser::consume() -> std::pair<ast::AST, Diagnostics> {
    reset(input_);
    ast::AST    ast;
    Diagnostics diagnostics;

    while (!current_token_is(syntax::TokenType::END)) {
        // Advance through any amount of semicolons
        const auto skip = [](syntax::TokenType tt) { return tt == syntax::TokenType::SEMICOLON; };
        if (skip(current_token_.type)) { while (skip(advance().type)); }
        if (current_token_is(syntax::TokenType::END)) { break; }

        // Comments are entirely discarded from the tree
        if (!current_token_is(syntax::TokenType::COMMENT)) {
            auto stmt = parse_statement();
            if (stmt) {
                ast.emplace_back(std::move(*stmt));
            } else {
                diagnostics.emplace_back(std::move(stmt.error()));

                // Errors should advance up to next logical end to prevent useless errors
                const auto stop_condition = [](syntax::TokenType tt) {
                    switch (tt) {
                    case syntax::TokenType::RBRACE:
                    case syntax::TokenType::SEMICOLON:
                    case syntax::TokenType::END:       return true;
                    default:                           return false;
                    }
                };
                while (!stop_condition(advance().type));
            }
        }
        advance();
    }

    return {std::move(ast), std::move(diagnostics)};
}

auto syntax::Parser::expect_peek(syntax::TokenType expected)
    -> Expected<std::monostate, syntax::ParserDiagnostic> {
    if (peek_token_is(expected)) {
        advance();
        return {};
    }
    return Unexpected{peek_error(expected)};
}

auto syntax::Parser::poll_current_precedence() const noexcept -> Precedence {
    return get_binding(current_token_.type)
        .transform([](const auto& binding) { return binding.second; })
        .value_or(Precedence::LOWEST);
}

auto syntax::Parser::poll_peek_precedence() const noexcept -> Precedence {
    return get_binding(peek_token_.type)
        .transform([](const auto& binding) { return binding.second; })
        .value_or(Precedence::LOWEST);
}

auto syntax::Parser::parse_statement() -> Expected<Box<ast::Statement>, syntax::ParserDiagnostic> {
    switch (current_token_.type) {
    case syntax::TokenType::VAR:
    case syntax::TokenType::CONST:
    case syntax::TokenType::COMPTIME:
    case syntax::TokenType::PUBLIC:
    case syntax::TokenType::EXTERN:
    case syntax::TokenType::EXPORT:     return ast::DeclStatement::parse(*this);
    case syntax::TokenType::BREAK:
    case syntax::TokenType::RETURN:
    case syntax::TokenType::CONTINUE:   return ast::JumpStatement::parse(*this);
    case syntax::TokenType::DEFER:      return ast::DeferStatement::parse(*this);
    case syntax::TokenType::IMPORT:     return ast::ImportStatement::parse(*this);
    case syntax::TokenType::LBRACE:     return ast::BlockStatement::parse(*this);
    case syntax::TokenType::UNDERSCORE: return ast::DiscardStatement::parse(*this);
    case syntax::TokenType::MODULE:     return ast::ModuleStatement::parse(*this);
    case syntax::TokenType::USING:      return ast::UsingStatement::parse(*this);
    default:                            return ast::ExpressionStatement::parse(*this);
    }
}

auto syntax::Parser::parse_expression(Precedence precedence)
    -> Expected<Box<ast::Expression>, syntax::ParserDiagnostic> {
    if (current_token_is(syntax::TokenType::END)) {
        return make_parser_unexpected(syntax::ParserError::END_OF_TOKEN_STREAM, current_token_);
    }

    const auto& prefix = poll_prefix_fn(current_token_.type);
    if (!prefix) {
        return make_parser_unexpected(fmt::format("No prefix parse function for {}({}) found",
                                                  magic_enum::enum_name(current_token_.type),
                                                  current_token_.slice),
                                      syntax::ParserError::MISSING_PREFIX_PARSER,
                                      current_token_);
    }
    auto lhs_expression = TRY((*prefix)(*this));

    while (!peek_token_is(syntax::TokenType::SEMICOLON) && precedence < poll_peek_precedence()) {
        const auto& infix = poll_infix_fn(peek_token_.type);
        if (!infix) { break; }
        advance();
        lhs_expression = TRY((*infix)(*this, std::move(lhs_expression)));
    }

    return lhs_expression;
}

[[nodiscard]] auto syntax::Parser::parse_restricted_statement(syntax::ParserError error)
    -> Expected<Box<ast::Statement>, syntax::ParserDiagnostic> {
    using namespace ast;
    auto clause = TRY(parse_statement());

    // The clause can only be a jump, block, or expression statement
    if (!clause->any<ExpressionStatement, JumpStatement, BlockStatement>()) {
        return make_parser_unexpected(error, clause->get_token());
    }
    return clause;
}

[[nodiscard]] auto syntax::Parser::try_parse_restricted_alternate(syntax::ParserError error)
    -> Expected<Optional<Box<ast::Statement>>, syntax::ParserDiagnostic> {
    if (peek_token_is(syntax::TokenType::ELSE)) {
        // Advance twice to actually look at the statement's first token
        advance(2);
        return TRY(parse_restricted_statement(error));
    }
    return std::nullopt;
}

using PrefixPair          = std::pair<syntax::TokenType, syntax::Parser::PrefixFn>;
constexpr auto PREFIX_FNS = []() {
    constexpr auto initial_prefixes = std::to_array<PrefixPair>({
        {syntax::TokenType::IDENT, ast::IdentifierExpression::parse},
        {syntax::TokenType::BYTE, ast::ByteExpression::parse},
        {syntax::TokenType::FLOAT, ast::FloatExpression::parse},
        {syntax::TokenType::DOUBLE, ast::DoubleExpression::parse},
        {syntax::TokenType::BANG, ast::UnaryExpression::parse},
        {syntax::TokenType::NOT, ast::UnaryExpression::parse},
        {syntax::TokenType::MINUS, ast::UnaryExpression::parse},
        {syntax::TokenType::PLUS, ast::UnaryExpression::parse},
        {syntax::TokenType::STAR, ast::DereferenceExpression::parse},
        {syntax::TokenType::BW_AND, ast::ReferenceExpression::parse},
        {syntax::TokenType::AND_MUT, ast::ReferenceExpression::parse},
        {syntax::TokenType::DOT, ast::ImplicitAccessExpression::parse},
        {syntax::TokenType::TRUE, ast::BoolExpression::parse},
        {syntax::TokenType::FALSE, ast::BoolExpression::parse},
        {syntax::TokenType::STRING, ast::StringExpression::parse},
        {syntax::TokenType::MULTILINE_STRING, ast::StringExpression::parse},
        {syntax::TokenType::LPAREN, ast::GroupedExpression::parse},
        {syntax::TokenType::IF, ast::IfExpression::parse},
        {syntax::TokenType::FUNCTION, ast::FunctionExpression::parse},
        {syntax::TokenType::PACKED, ast::StructExpression::parse},
        {syntax::TokenType::STRUCT, ast::StructExpression::parse},
        {syntax::TokenType::UNION, ast::UnionExpression::parse},
        {syntax::TokenType::ENUM, ast::EnumExpression::parse},
        {syntax::TokenType::MATCH, ast::MatchExpression::parse},
        {syntax::TokenType::LBRACKET, ast::ArrayExpression::parse},
        {syntax::TokenType::FOR, ast::ForLoopExpression::parse},
        {syntax::TokenType::WHILE, ast::WhileLoopExpression::parse},
        {syntax::TokenType::DO, ast::DoWhileLoopExpression::parse},
        {syntax::TokenType::LOOP, ast::InfiniteLoopExpression::parse},
    });

    constexpr auto total_ints = static_cast<usize>(syntax::TokenType::UZINT_16) -
                                static_cast<usize>(syntax::TokenType::INT_2) + 1;
    std::array<PrefixPair, total_ints> int_prefixes;
    usize                              cursor = 0;
    for (auto i = std::to_underlying(syntax::TokenType::INT_2);
         i <= std::to_underlying(syntax::TokenType::UZINT_16);
         ++i, ++cursor) {
        const auto tt = static_cast<syntax::TokenType>(i);
        using namespace token_type;
        switch (to_int_category(tt)) {
        case IntegerCategory::SIGNED_BASE:
            int_prefixes[cursor] = {tt, ast::SignedIntegerExpression::parse};
            break;
        case IntegerCategory::SIGNED_WIDE:
            int_prefixes[cursor] = {tt, ast::SignedLongIntegerExpression::parse};
            break;
        case IntegerCategory::SIGNED_SIZE:
            int_prefixes[cursor] = {tt, ast::ISizeIntegerExpression::parse};
            break;
        case IntegerCategory::UNSIGNED_BASE:
            int_prefixes[cursor] = {tt, ast::UnsignedIntegerExpression::parse};
            break;
        case IntegerCategory::UNSIGNED_WIDE:
            int_prefixes[cursor] = {tt, ast::UnsignedLongIntegerExpression::parse};
            break;
        case IntegerCategory::UNSIGNED_SIZE:
            int_prefixes[cursor] = {tt, ast::USizeIntegerExpression::parse};
            break;
        }
    }

    constexpr auto primitive_prefixes =
        ALL_PRIMITIVES | std::views::transform([](syntax::TokenType tt) -> PrefixPair {
            return {tt, ast::IdentifierExpression::parse};
        });

    constexpr auto materialized_primitives =
        array::materialize_sized_view<ALL_PRIMITIVES.size()>(primitive_prefixes);

    constexpr auto builtins_prefixes =
        ALL_BUILTINS | std::views::transform([](const auto& builtin) -> PrefixPair {
            return {builtin.second, ast::IdentifierExpression::parse};
        });

    constexpr auto materialized_builtins =
        array::materialize_sized_view<ALL_BUILTINS.size()>(builtins_prefixes);

    auto prefix_fns = array::concat(
        initial_prefixes, materialized_primitives, materialized_builtins, int_prefixes);
    std::ranges::sort(prefix_fns, {}, &PrefixPair::first);
    return prefix_fns;
}();

constexpr auto syntax::Parser::poll_prefix_fn(syntax::TokenType tt) noexcept
    -> Optional<const PrefixFn&> {
    const auto it = std::ranges::lower_bound(PREFIX_FNS, tt, {}, &PrefixPair::first);
    if (it == PREFIX_FNS.end() || it->first != tt) { return std::nullopt; }
    return Optional<const PrefixFn&>{it->second};
}

using InfixPair          = std::pair<syntax::TokenType, syntax::Parser::InfixFn>;
constexpr auto INFIX_FNS = []() {
    auto infix_fns = std::to_array<InfixPair>({
        {syntax::TokenType::PLUS, ast::BinaryExpression::parse},
        {syntax::TokenType::MINUS, ast::BinaryExpression::parse},
        {syntax::TokenType::STAR, ast::BinaryExpression::parse},
        {syntax::TokenType::SLASH, ast::BinaryExpression::parse},
        {syntax::TokenType::PERCENT, ast::BinaryExpression::parse},
        {syntax::TokenType::LT, ast::BinaryExpression::parse},
        {syntax::TokenType::LT_EQ, ast::BinaryExpression::parse},
        {syntax::TokenType::GT, ast::BinaryExpression::parse},
        {syntax::TokenType::GT_EQ, ast::BinaryExpression::parse},
        {syntax::TokenType::EQ, ast::BinaryExpression::parse},
        {syntax::TokenType::NEQ, ast::BinaryExpression::parse},
        {syntax::TokenType::BOOLEAN_AND, ast::BinaryExpression::parse},
        {syntax::TokenType::BOOLEAN_OR, ast::BinaryExpression::parse},
        {syntax::TokenType::BW_AND, ast::BinaryExpression::parse},
        {syntax::TokenType::BW_OR, ast::BinaryExpression::parse},
        {syntax::TokenType::XOR, ast::BinaryExpression::parse},
        {syntax::TokenType::SHR, ast::BinaryExpression::parse},
        {syntax::TokenType::SHL, ast::BinaryExpression::parse},
        {syntax::TokenType::DOT, ast::DotExpression::parse},
        {syntax::TokenType::DOT_DOT, ast::RangeExpression::parse},
        {syntax::TokenType::DOT_DOT_EQ, ast::RangeExpression::parse},
        {syntax::TokenType::LPAREN, ast::CallExpression::parse},
        {syntax::TokenType::LBRACKET, ast::IndexExpression::parse},
        {syntax::TokenType::ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::PLUS_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::MINUS_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::STAR_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::SLASH_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::PERCENT_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::BW_AND_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::BW_OR_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::SHL_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::SHR_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::NOT_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::XOR_ASSIGN, ast::AssignmentExpression::parse},
        {syntax::TokenType::COLON_COLON, ast::ScopeResolutionExpression::parse},
    });

    std::ranges::sort(infix_fns, {}, &InfixPair::first);
    return infix_fns;
}();

constexpr auto syntax::Parser::poll_infix_fn(syntax::TokenType tt) noexcept
    -> Optional<const InfixFn&> {
    const auto it = std::ranges::lower_bound(INFIX_FNS, tt, {}, &InfixPair::first);
    if (it == INFIX_FNS.end() || it->first != tt) { return std::nullopt; }
    return Optional<const InfixFn&>{it->second};
}

auto syntax::Parser::tt_mismatch_error(syntax::TokenType expected, const syntax::Token& actual)
    -> syntax::ParserDiagnostic {
    return syntax::ParserDiagnostic{fmt::format("Expected token {}, found {}",
                                                magic_enum::enum_name(expected),
                                                magic_enum::enum_name(actual.type)),
                                    syntax::ParserError::UNEXPECTED_TOKEN,
                                    actual};
}

} // namespace porpoise::syntax
