#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "syntax/keywords.hpp"
#include "syntax/parser.hpp"
#include "syntax/precedence.hpp"
#include "syntax/token.hpp"

#include "ast/ast.hpp"

#include "enum.hpp"

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

auto Parser::consume() -> std::pair<ast::AST, ParserDiagnostics> {
    reset(input_);
    ast::AST          ast;
    ParserDiagnostics diagnostics;

    while (!current_token_is(TokenType::END)) {
        // Advance through any amount of semicolons
        const auto skip = [](TokenType tt) { return tt == TokenType::SEMICOLON; };
        if (skip(current_token_.type)) { while (skip(advance().type)); }
        if (current_token_is(TokenType::END)) { break; }

        // Comments are entirely discarded from the tree
        if (!current_token_is(TokenType::COMMENT)) {
            auto stmt = parse_statement(true);
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

auto Parser::expect_peek(TokenType expected) -> Result<Unit, ParserDiagnostic> {
    if (peek_token_is(expected)) {
        advance();
        return {};
    }
    return Err{peek_error(expected)};
}

auto Parser::peek_error(TokenType expected) -> ParserDiagnostic {
    return ParserDiagnostic{fmt::format("Expected token {}, found {}",
                                        magic_enum::enum_name(expected),
                                        magic_enum::enum_name(peek_token_.type)),
                            ParserError::UNEXPECTED_TOKEN,
                            peek_token_};
}

auto Parser::get_current_precedence() const noexcept
    -> std::pair<Precedence, opt::Option<Binding>> {
    return Binding::try_get_from(current_token_.type)
        .transform([](const auto& binding) {
            return std::pair{binding.precedence, opt::Option<Binding>{binding}};
        })
        .value_or(std::pair{Precedence::LOWEST, opt::none});
}

auto Parser::get_peek_precedence() const noexcept -> std::pair<Precedence, opt::Option<Binding>> {
    return Binding::try_get_from(peek_token_.type)
        .transform([](const auto& binding) {
            return std::pair{binding.precedence, opt::Option<Binding>{binding}};
        })
        .value_or(std::pair{Precedence::LOWEST, opt::none});
}

auto Parser::parse_statement(bool require_semicolon)
    -> Result<mem::Box<ast::Statement>, ParserDiagnostic> {
    if (current_token_.is_decl_token()) { return ast::DeclStatement::parse(*this); }
    switch (current_token_.type) {
    case TokenType::LBRACE:     return ast::BlockStatement::parse(*this);
    case TokenType::BREAK:      return ast::BreakStatement::parse(*this);
    case TokenType::CONTINUE:   return ast::ContinueStatement::parse(*this);
    case TokenType::DEFER:      return ast::DeferStatement::parse(*this);
    case TokenType::UNDERSCORE: return ast::DiscardStatement::parse(*this);
    case TokenType::IMPORT:     return ast::ImportStatement::parse(*this);
    case TokenType::MODULE:     return ast::ModuleStatement::parse(*this);
    case TokenType::RETURN:     return ast::ReturnStatement::parse(*this);
    case TokenType::TEST:       return ast::TestStatement::parse(*this);
    case TokenType::USING:      return ast::UsingStatement::parse(*this);
    default:                    return ast::ExpressionStatement::parse(*this, require_semicolon);
    }
}

auto Parser::parse_expression(Precedence precedence)
    -> Result<mem::Box<ast::Expression>, ParserDiagnostic> {
    if (current_token_is(TokenType::END)) {
        return make_parser_err(ParserError::END_OF_TOKEN_STREAM, current_token_);
    }

    const auto& prefix = try_get_prefix_fn(current_token_.type);
    if (!prefix) {
        return make_parser_err(fmt::format("No prefix parse function for {}({}) found",
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

[[nodiscard]] auto Parser::parse_restricted_statement(ParserError error, bool require_semicolon)
    -> Result<mem::Box<ast::Statement>, ParserDiagnostic> {
    auto clause = TRY(parse_statement(require_semicolon));

    // The clause can only be a jump, block, or expression statement
    if (!clause->any<ast::ExpressionStatement,
                     ast::BreakStatement,
                     ast::ContinueStatement,
                     ast::ReturnStatement,
                     ast::BlockStatement>()) {
        return make_parser_err(error, clause->get_token());
    }
    return clause;
}

[[nodiscard]] auto Parser::try_parse_restricted_alternate(ParserError error, bool require_semicolon)
    -> Result<mem::NullableBox<ast::Statement>, ParserDiagnostic> {
    if (peek_token_is(TokenType::ELSE)) {
        // Advance twice to actually look at the statement's first token
        advance(2);
        return mem::nullable_box_from(TRY(parse_restricted_statement(error, require_semicolon)));
    }
    return nullptr;
}

auto Parser::parse_member_decls(ast::MemberValidator validator)
    -> Result<ast::Members, ParserDiagnostic> {
    ast::Members members;
    while (!peek_token_is(syntax::TokenType::RBRACE) && !peek_token_is(syntax::TokenType::END)) {
        advance();
        auto member = TRY(parse_statement(true));
        if (!member->is<ast::DeclStatement>()) {
            return make_parser_err(syntax::ParserError::INVALID_MEMBER, member->get_token());
        }

        // Check the decl against the validator if provided
        if (validator && !validator(ast::Node::as<ast::DeclStatement>(*member))) {
            return make_parser_err(syntax::ParserError::INVALID_MEMBER, member->get_token());
        }
        members.emplace_back(ast::Node::downcast<ast::DeclStatement>(std::move(member)));
    }
    return members;
}

constexpr auto PREFIX_FNS = [] {
    EnumMap<TokenType, Parser::PrefixFn> fns;

    fns[TokenType::IDENT]            = ast::IdentifierExpression::parse;
    fns[TokenType::U8]               = ast::U8Expression::parse;
    fns[TokenType::F32]              = ast::F32Expression::parse;
    fns[TokenType::F64]              = ast::F64Expression::parse;
    fns[TokenType::BANG]             = ast::UnaryExpression::parse;
    fns[TokenType::NOT]              = ast::UnaryExpression::parse;
    fns[TokenType::MINUS]            = ast::UnaryExpression::parse;
    fns[TokenType::PLUS]             = ast::UnaryExpression::parse;
    fns[TokenType::STAR]             = ast::DereferenceExpression::parse;
    fns[TokenType::BW_AND]           = ast::ReferenceExpression::parse;
    fns[TokenType::AND_MUT]          = ast::ReferenceExpression::parse;
    fns[TokenType::DOT]              = ast::ImplicitAccessExpression::parse;
    fns[TokenType::BOOLEAN_TRUE]     = ast::BoolExpression::parse;
    fns[TokenType::BOOLEAN_FALSE]    = ast::BoolExpression::parse;
    fns[TokenType::LBRACE]           = ast::VoidExpression::parse;
    fns[TokenType::STRING]           = ast::StringExpression::parse;
    fns[TokenType::MULTILINE_STRING] = ast::StringExpression::parse;
    fns[TokenType::LPAREN]           = ast::GroupedExpression::parse;
    fns[TokenType::IF]               = ast::IfExpression::parse;
    fns[TokenType::FUNCTION]         = ast::FunctionExpression::parse;
    fns[TokenType::PACKED]           = ast::StructExpression::parse;
    fns[TokenType::STRUCT]           = ast::StructExpression::parse;
    fns[TokenType::UNION]            = ast::UnionExpression::parse;
    fns[TokenType::ENUM]             = ast::EnumExpression::parse;
    fns[TokenType::MATCH]            = ast::MatchExpression::parse;
    fns[TokenType::LBRACKET]         = ast::ArrayExpression::parse;
    fns[TokenType::FOR]              = ast::ForLoopExpression::parse;
    fns[TokenType::WHILE]            = ast::WhileLoopExpression::parse;
    fns[TokenType::DO]               = ast::DoWhileLoopExpression::parse;
    fns[TokenType::LOOP]             = ast::InfiniteLoopExpression::parse;

    for (const auto tt : enum_range<TokenType::INT_2, TokenType::UZINT_16>()) {
        using token_type::IntegerCategory;
        switch (token_type::to_int_category(tt)) {
        case IntegerCategory::SIGNED_BASE:   fns[tt] = ast::I32Expression::parse; break;
        case IntegerCategory::SIGNED_WIDE:   fns[tt] = ast::I64Expression::parse; break;
        case IntegerCategory::SIGNED_SIZE:   fns[tt] = ast::ISizeExpression::parse; break;
        case IntegerCategory::UNSIGNED_BASE: fns[tt] = ast::U32Expression::parse; break;
        case IntegerCategory::UNSIGNED_WIDE: fns[tt] = ast::U64Expression::parse; break;
        case IntegerCategory::UNSIGNED_SIZE: fns[tt] = ast::USizeExpression::parse; break;
        }
    }

    for (const auto tt : ALL_PRIMITIVES) { fns[tt] = ast::IdentifierExpression::parse; }
    for (const auto& [_, tt] : ALL_BUILTINS) { fns[tt] = ast::IdentifierExpression::parse; }

    return fns;
}();

auto Parser::try_get_prefix_fn(TokenType tt) noexcept -> opt::Option<PrefixFn> {
    return PREFIX_FNS.get_opt(tt);
}

constexpr auto INFIX_FNS = [] {
    EnumMap<TokenType, Parser::InfixFn> fns;

    fns[TokenType::PLUS]           = ast::BinaryExpression::parse;
    fns[TokenType::MINUS]          = ast::BinaryExpression::parse;
    fns[TokenType::STAR]           = ast::BinaryExpression::parse;
    fns[TokenType::SLASH]          = ast::BinaryExpression::parse;
    fns[TokenType::PERCENT]        = ast::BinaryExpression::parse;
    fns[TokenType::LT]             = ast::BinaryExpression::parse;
    fns[TokenType::LT_EQ]          = ast::BinaryExpression::parse;
    fns[TokenType::GT]             = ast::BinaryExpression::parse;
    fns[TokenType::GT_EQ]          = ast::BinaryExpression::parse;
    fns[TokenType::EQ]             = ast::BinaryExpression::parse;
    fns[TokenType::NEQ]            = ast::BinaryExpression::parse;
    fns[TokenType::BOOLEAN_AND]    = ast::BinaryExpression::parse;
    fns[TokenType::BOOLEAN_OR]     = ast::BinaryExpression::parse;
    fns[TokenType::BW_AND]         = ast::BinaryExpression::parse;
    fns[TokenType::BW_OR]          = ast::BinaryExpression::parse;
    fns[TokenType::XOR]            = ast::BinaryExpression::parse;
    fns[TokenType::SHR]            = ast::BinaryExpression::parse;
    fns[TokenType::SHL]            = ast::BinaryExpression::parse;
    fns[TokenType::DOT]            = ast::DotExpression::parse;
    fns[TokenType::DOT_DOT]        = ast::RangeExpression::parse;
    fns[TokenType::DOT_DOT_EQ]     = ast::RangeExpression::parse;
    fns[TokenType::LPAREN]         = ast::CallExpression::parse;
    fns[TokenType::LBRACKET]       = ast::IndexExpression::parse;
    fns[TokenType::LBRACE]         = ast::InitializerExpression::parse;
    fns[TokenType::ASSIGN]         = ast::AssignmentExpression::parse;
    fns[TokenType::PLUS_ASSIGN]    = ast::AssignmentExpression::parse;
    fns[TokenType::MINUS_ASSIGN]   = ast::AssignmentExpression::parse;
    fns[TokenType::STAR_ASSIGN]    = ast::AssignmentExpression::parse;
    fns[TokenType::SLASH_ASSIGN]   = ast::AssignmentExpression::parse;
    fns[TokenType::PERCENT_ASSIGN] = ast::AssignmentExpression::parse;
    fns[TokenType::BW_AND_ASSIGN]  = ast::AssignmentExpression::parse;
    fns[TokenType::BW_OR_ASSIGN]   = ast::AssignmentExpression::parse;
    fns[TokenType::SHL_ASSIGN]     = ast::AssignmentExpression::parse;
    fns[TokenType::SHR_ASSIGN]     = ast::AssignmentExpression::parse;
    fns[TokenType::NOT_ASSIGN]     = ast::AssignmentExpression::parse;
    fns[TokenType::XOR_ASSIGN]     = ast::AssignmentExpression::parse;
    fns[TokenType::COLON_COLON]    = ast::ScopeResolutionExpression::parse;
    fns[TokenType::COLON]          = ast::LabelExpression::parse;

    return fns;
}();

auto Parser::try_get_poll_infix_fn(TokenType tt) noexcept -> opt::Option<InfixFn> {
    return INFIX_FNS.get_opt(tt);
}

} // namespace porpoise::syntax
