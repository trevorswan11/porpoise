#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "syntax/keywords.hpp"
#include "syntax/parser.hpp"
#include "syntax/precedence.hpp"
#include "syntax/token.hpp"

#include "ast/ast.hpp"

namespace porpoise::syntax {

Parser::Checkpoint::Checkpoint(const Parser& parser) noexcept
    : snapshot_{parser.lexer_}, current_{parser.current_token_}, peek_{parser.peek_token_} {}

auto Parser::reset(std::string_view input) noexcept -> void { *this = Parser{input}; }

auto Parser::advance(u8 times) noexcept -> const Token& {
    for (u8 i = 0; i < times; ++i) {
        if (current_token_.type == TokenType::END &&
            (input_.empty() && current_token_.is_at_start())) {
            break;
        }

        current_token_ = peek_token_;
        peek_token_    = lexer_.advance();
    }
    return current_token_;
}

auto Parser::consume() -> std::pair<ast::AST, Diagnostics> {
    reset(input_);
    ast::AST    ast;
    Diagnostics diagnostics;

    while (!current_token_is(TokenType::END)) {
        // Advance through any amount of semicolons
        const auto skip = [](TokenType tt) { return tt == TokenType::SEMICOLON; };
        if (skip(current_token_.type)) { while (skip(advance().type)); }
        if (current_token_is(TokenType::END)) { break; }

        // Comments are entirely discarded from the tree
        if (!current_token_is(TokenType::COMMENT)) {
            auto stmt = parse_statement();
            if (stmt) {
                ast.emplace_back(std::move(*stmt));
            } else {
                diagnostics.emplace_back(std::move(stmt.error()));

                // Errors should advance up to next logical end to prevent useless errors
                const auto stop_condition = [](TokenType tt) {
                    switch (tt) {
                    case TokenType::RBRACE:
                    case TokenType::SEMICOLON:
                    case TokenType::END:       return true;
                    default:                   return false;
                    }
                };
                while (!stop_condition(advance().type));
            }
        }
        advance();
    }

    return {std::move(ast), std::move(diagnostics)};
}

auto Parser::expect_peek(TokenType expected) -> Expected<Unit, ParserDiagnostic> {
    if (peek_token_is(expected)) {
        advance();
        return {};
    }
    return Unexpected{peek_error(expected)};
}

auto Parser::peek_error(TokenType expected) -> ParserDiagnostic {
    return ParserDiagnostic{fmt::format("Expected token {}, found {}",
                                        magic_enum::enum_name(expected),
                                        magic_enum::enum_name(peek_token_.type)),
                            ParserError::UNEXPECTED_TOKEN,
                            peek_token_};
}

auto Parser::get_current_precedence() const noexcept -> std::pair<Precedence, Optional<Binding>> {
    return Binding::try_get_from(current_token_.type)
        .transform([](const auto& binding) {
            return std::pair{binding.precedence, Optional<Binding>{binding}};
        })
        .value_or(std::pair{Precedence::LOWEST, std::nullopt});
}

auto Parser::get_peek_precedence() const noexcept -> std::pair<Precedence, Optional<Binding>> {
    return Binding::try_get_from(peek_token_.type)
        .transform([](const auto& binding) {
            return std::pair{binding.precedence, Optional<Binding>{binding}};
        })
        .value_or(std::pair{Precedence::LOWEST, std::nullopt});
}

auto Parser::parse_statement() -> Expected<mem::Box<ast::Statement>, ParserDiagnostic> {
    if (current_token_.is_decl_token()) { return ast::DeclStatement::parse(*this); }
    switch (current_token_.type) {
    case TokenType::BREAK:
    case TokenType::RETURN:
    case TokenType::CONTINUE:   return ast::JumpStatement::parse(*this);
    case TokenType::DEFER:      return ast::DeferStatement::parse(*this);
    case TokenType::IMPORT:     return ast::ImportStatement::parse(*this);
    case TokenType::LBRACE:     return ast::BlockStatement::parse(*this);
    case TokenType::UNDERSCORE: return ast::DiscardStatement::parse(*this);
    case TokenType::MODULE:     return ast::ModuleStatement::parse(*this);
    case TokenType::USING:      return ast::UsingStatement::parse(*this);
    case TokenType::TEST:       return ast::TestStatement::parse(*this);
    default:                    return ast::ExpressionStatement::parse(*this);
    }
}

auto Parser::parse_expression(Precedence precedence)
    -> Expected<mem::Box<ast::Expression>, ParserDiagnostic> {
    if (current_token_is(TokenType::END)) {
        return make_parser_unexpected(ParserError::END_OF_TOKEN_STREAM, current_token_);
    }

    const auto& prefix = try_get_prefix_fn(current_token_.type);
    if (!prefix) {
        return make_parser_unexpected(fmt::format("No prefix parse function for {}({}) found",
                                                  magic_enum::enum_name(current_token_.type),
                                                  current_token_.slice),
                                      ParserError::MISSING_PREFIX_PARSER,
                                      current_token_);
    }
    auto lhs_expression = TRY((*prefix)(*this));

    while (!peek_token_is(TokenType::SEMICOLON) && precedence < get_peek_precedence().first) {
        const auto& infix = try_get_poll_infix_fn(peek_token_.type);
        if (!infix) { break; }
        advance();
        lhs_expression = TRY((*infix)(*this, std::move(lhs_expression)));
    }

    return lhs_expression;
}

[[nodiscard]] auto Parser::parse_restricted_statement(ParserError error)
    -> Expected<mem::Box<ast::Statement>, ParserDiagnostic> {
    auto clause = TRY(parse_statement());

    // The clause can only be a jump, block, or expression statement
    if (!clause->any<ast::ExpressionStatement, ast::JumpStatement, ast::BlockStatement>()) {
        return make_parser_unexpected(error, clause->get_token());
    }
    return clause;
}

[[nodiscard]] auto Parser::try_parse_restricted_alternate(ParserError error)
    -> Expected<mem::NullableBox<ast::Statement>, ParserDiagnostic> {
    if (peek_token_is(TokenType::ELSE)) {
        // Advance twice to actually look at the statement's first token
        advance(2);
        return mem::nullable_box_from(TRY(parse_restricted_statement(error)));
    }
    return nullptr;
}

auto Parser::parse_member_decls(ast::MemberValidator validator)
    -> Expected<ast::Members, ParserDiagnostic> {
    ast::Members members;
    while (!peek_token_is(syntax::TokenType::RBRACE) && !peek_token_is(syntax::TokenType::END)) {
        advance();
        auto member = TRY(parse_statement());
        if (!member->is<ast::DeclStatement>()) {
            return make_parser_unexpected(syntax::ParserError::INVALID_MEMBER, member->get_token());
        }

        // Check the decl against the validator if provided
        if (validator && !validator(ast::Node::as<ast::DeclStatement>(*member))) {
            return make_parser_unexpected(syntax::ParserError::INVALID_MEMBER, member->get_token());
        }
        members.emplace_back(ast::Node::downcast<ast::DeclStatement>(std::move(member)));
    }
    return members;
}

constexpr auto PREFIX_FNS = [] {
    using enum TokenType;
    std::array<Parser::PrefixFn, TOKEN_TYPE_COUNT> fns;
    fns.fill(nullptr);

    fns[static_cast<usize>(IDENT)]            = ast::IdentifierExpression::parse;
    fns[static_cast<usize>(U8)]               = ast::U8Expression::parse;
    fns[static_cast<usize>(F32)]              = ast::F32Expression::parse;
    fns[static_cast<usize>(F64)]              = ast::F64Expression::parse;
    fns[static_cast<usize>(BANG)]             = ast::UnaryExpression::parse;
    fns[static_cast<usize>(NOT)]              = ast::UnaryExpression::parse;
    fns[static_cast<usize>(MINUS)]            = ast::UnaryExpression::parse;
    fns[static_cast<usize>(PLUS)]             = ast::UnaryExpression::parse;
    fns[static_cast<usize>(STAR)]             = ast::DereferenceExpression::parse;
    fns[static_cast<usize>(BW_AND)]           = ast::ReferenceExpression::parse;
    fns[static_cast<usize>(AND_MUT)]          = ast::ReferenceExpression::parse;
    fns[static_cast<usize>(DOT)]              = ast::ImplicitAccessExpression::parse;
    fns[static_cast<usize>(BOOLEAN_TRUE)]     = ast::BoolExpression::parse;
    fns[static_cast<usize>(BOOLEAN_FALSE)]    = ast::BoolExpression::parse;
    fns[static_cast<usize>(LBRACE)]           = ast::VoidExpression::parse;
    fns[static_cast<usize>(STRING)]           = ast::StringExpression::parse;
    fns[static_cast<usize>(MULTILINE_STRING)] = ast::StringExpression::parse;
    fns[static_cast<usize>(LPAREN)]           = ast::GroupedExpression::parse;
    fns[static_cast<usize>(IF)]               = ast::IfExpression::parse;
    fns[static_cast<usize>(FUNCTION)]         = ast::FunctionExpression::parse;
    fns[static_cast<usize>(PACKED)]           = ast::StructExpression::parse;
    fns[static_cast<usize>(STRUCT)]           = ast::StructExpression::parse;
    fns[static_cast<usize>(UNION)]            = ast::UnionExpression::parse;
    fns[static_cast<usize>(ENUM)]             = ast::EnumExpression::parse;
    fns[static_cast<usize>(MATCH)]            = ast::MatchExpression::parse;
    fns[static_cast<usize>(LBRACKET)]         = ast::ArrayExpression::parse;
    fns[static_cast<usize>(FOR)]              = ast::ForLoopExpression::parse;
    fns[static_cast<usize>(WHILE)]            = ast::WhileLoopExpression::parse;
    fns[static_cast<usize>(DO)]               = ast::DoWhileLoopExpression::parse;
    fns[static_cast<usize>(LOOP)]             = ast::InfiniteLoopExpression::parse;

    for (auto i = static_cast<usize>(INT_2); i <= static_cast<usize>(UZINT_16); ++i) {
        const auto tt = static_cast<TokenType>(i);
        using token_type::IntegerCategory;
        switch (token_type::to_int_category(tt)) {
        case IntegerCategory::SIGNED_BASE:   fns[i] = ast::I32Expression::parse; break;
        case IntegerCategory::SIGNED_WIDE:   fns[i] = ast::I64Expression::parse; break;
        case IntegerCategory::SIGNED_SIZE:   fns[i] = ast::ISizeExpression::parse; break;
        case IntegerCategory::UNSIGNED_BASE: fns[i] = ast::U32Expression::parse; break;
        case IntegerCategory::UNSIGNED_WIDE: fns[i] = ast::U64Expression::parse; break;
        case IntegerCategory::UNSIGNED_SIZE: fns[i] = ast::USizeExpression::parse; break;
        }
    }

    for (const auto tt : ALL_PRIMITIVES) {
        fns[static_cast<usize>(tt)] = ast::IdentifierExpression::parse;
    }

    for (const auto builtin : ALL_BUILTINS) {
        fns[static_cast<usize>(builtin.second)] = ast::IdentifierExpression::parse;
    }

    return fns;
}();

auto Parser::try_get_prefix_fn(TokenType tt) noexcept -> Optional<PrefixFn> {
    const auto fn = PREFIX_FNS[static_cast<usize>(tt)];
    return fn ? Optional<PrefixFn>{fn} : std::nullopt;
}

constexpr auto INFIX_FNS = [] {
    using enum TokenType;
    std::array<Parser::InfixFn, TOKEN_TYPE_COUNT> fns;
    fns.fill(nullptr);

    fns[static_cast<usize>(PLUS)]           = ast::BinaryExpression::parse;
    fns[static_cast<usize>(MINUS)]          = ast::BinaryExpression::parse;
    fns[static_cast<usize>(STAR)]           = ast::BinaryExpression::parse;
    fns[static_cast<usize>(SLASH)]          = ast::BinaryExpression::parse;
    fns[static_cast<usize>(PERCENT)]        = ast::BinaryExpression::parse;
    fns[static_cast<usize>(LT)]             = ast::BinaryExpression::parse;
    fns[static_cast<usize>(LT_EQ)]          = ast::BinaryExpression::parse;
    fns[static_cast<usize>(GT)]             = ast::BinaryExpression::parse;
    fns[static_cast<usize>(GT_EQ)]          = ast::BinaryExpression::parse;
    fns[static_cast<usize>(EQ)]             = ast::BinaryExpression::parse;
    fns[static_cast<usize>(NEQ)]            = ast::BinaryExpression::parse;
    fns[static_cast<usize>(BOOLEAN_AND)]    = ast::BinaryExpression::parse;
    fns[static_cast<usize>(BOOLEAN_OR)]     = ast::BinaryExpression::parse;
    fns[static_cast<usize>(BW_AND)]         = ast::BinaryExpression::parse;
    fns[static_cast<usize>(BW_OR)]          = ast::BinaryExpression::parse;
    fns[static_cast<usize>(XOR)]            = ast::BinaryExpression::parse;
    fns[static_cast<usize>(SHR)]            = ast::BinaryExpression::parse;
    fns[static_cast<usize>(SHL)]            = ast::BinaryExpression::parse;
    fns[static_cast<usize>(DOT)]            = ast::DotExpression::parse;
    fns[static_cast<usize>(DOT_DOT)]        = ast::RangeExpression::parse;
    fns[static_cast<usize>(DOT_DOT_EQ)]     = ast::RangeExpression::parse;
    fns[static_cast<usize>(LPAREN)]         = ast::CallExpression::parse;
    fns[static_cast<usize>(LBRACKET)]       = ast::IndexExpression::parse;
    fns[static_cast<usize>(LBRACE)]         = ast::InitializerExpression::parse;
    fns[static_cast<usize>(ASSIGN)]         = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(PLUS_ASSIGN)]    = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(MINUS_ASSIGN)]   = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(STAR_ASSIGN)]    = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(SLASH_ASSIGN)]   = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(PERCENT_ASSIGN)] = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(BW_AND_ASSIGN)]  = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(BW_OR_ASSIGN)]   = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(SHL_ASSIGN)]     = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(SHR_ASSIGN)]     = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(NOT_ASSIGN)]     = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(XOR_ASSIGN)]     = ast::AssignmentExpression::parse;
    fns[static_cast<usize>(COLON_COLON)]    = ast::ScopeResolutionExpression::parse;

    return fns;
}();

auto Parser::try_get_poll_infix_fn(TokenType tt) noexcept -> Optional<InfixFn> {
    const auto fn = INFIX_FNS[static_cast<usize>(tt)];
    return fn ? Optional<InfixFn>{fn} : std::nullopt;
}

} // namespace porpoise::syntax
