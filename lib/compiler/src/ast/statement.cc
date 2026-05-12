#include "ast/statement.hh"

#include "syntax/parser.hh"

#include "enum.hh"

namespace porpoise::ast {

auto BlockStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    Statements statements;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        statements.emplace_back(TRY(parser.parse_statement(true)));
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    return parser.add_stmt(start_token, BlockStatement{std::move(statements)});
}

auto BreakStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    // Labels are optional
    opt::Option<IdentifierHandle> label;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        label.emplace(TRY(IdentifierExpression::parse(parser)));
    }

    // Values can be present but must be associated with a label
    opt::Option<ExpressionHandle> value;
    if (!parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        parser.advance();
        value.emplace(TRY(parser.parse_expression()));
    }

    if (value && !label) {
        return make_syntax_err(syntax::Error::VALUED_BREAK_MISSING_LABEL, start_token);
    }
    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return parser.add_stmt(start_token, BreakStatement{label, value});
}

auto ContinueStatement::parse(syntax::Parser& parser)
    -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    // Labels are optional
    opt::Option<IdentifierHandle> label;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        label.emplace(TRY(IdentifierExpression::parse(parser)));
    }

    // Values can never be present in a continue
    if (!parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_syntax_err(syntax::Error::VALUED_CONTINUE, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return parser.add_stmt(start_token, ContinueStatement{label});
}

namespace {

using ModifierMapping          = std::pair<syntax::TokenType, DeclModifiers>;
constexpr auto LEGAL_MODIFIERS = [] {
    using TokenType = syntax::TokenType;
    EnumMap<TokenType, opt::Option<DeclModifiers>> modifiers{opt::none};
    modifiers[TokenType::VAR]       = DeclModifiers::VARIABLE;
    modifiers[TokenType::CONSTANT]  = DeclModifiers::CONSTANT;
    modifiers[TokenType::CONSTEXPR] = DeclModifiers::CONSTEXPR;
    modifiers[TokenType::PUBLIC]    = DeclModifiers::PUBLIC;
    modifiers[TokenType::EXTERN]    = DeclModifiers::EXTERN;
    modifiers[TokenType::EXPORT]    = DeclModifiers::EXPORT;
    modifiers[TokenType::STATIC]    = DeclModifiers::STATIC;
    return modifiers;
}();

[[nodiscard]] constexpr auto validate_modifiers(DeclModifiers modifiers) noexcept -> bool {
    // Exactly one mutability flag must be set
    const auto valid_mut = std::popcount(std::to_underlying(
                               modifiers & (DeclModifiers::VARIABLE | DeclModifiers::CONSTANT |
                                            DeclModifiers::CONSTEXPR))) == 1;

    // Comptime values cannot be known at link time, obviously
    const auto valid_constexpr =
        std::popcount(std::to_underlying(modifiers &
                                         (DeclModifiers::EXTERN | DeclModifiers::CONSTEXPR))) <= 1;

    // At most one ABI flag can be set
    const auto valid_abi = std::popcount(std::to_underlying(
                               modifiers & (DeclModifiers::EXTERN | DeclModifiers::EXPORT))) <= 1;
    return valid_mut && valid_constexpr && valid_abi;
}

} // namespace

auto DeclStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    auto       modifiers   = LEGAL_MODIFIERS[start_token.type].value();

    opt::Option<DeclModifiers> current_modifier;
    while ((current_modifier = LEGAL_MODIFIERS[parser.get_peek_token().type])) {
        parser.advance();
        if (modifiers_has(modifiers, *current_modifier)) {
            return make_syntax_err(syntax::Error::DUPLICATE_DECL_MODIFIER,
                                   parser.get_current_token());
        }
        modifiers |= *current_modifier;
    }

    if (!validate_modifiers(modifiers)) {
        return make_syntax_err(syntax::Error::ILLEGAL_DECL_MODIFIERS, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    const IdentifierHandle decl_name          = TRY(IdentifierExpression::parse(parser));
    const auto [decl_type, value_initialized] = TRY(TypeExpression::parse(parser));

    opt::Option<ExpressionHandle> decl_value;
    if (value_initialized) {
        decl_value.emplace(TRY(parser.parse_expression()));

        // If there is a value, then there cannot be an extern keyword
        if (modifiers_has(modifiers, DeclModifiers::EXTERN)) {
            return make_syntax_err(syntax::Error::EXTERN_VALUE_INITIALIZED, start_token);
        }
    } else if ((modifiers_has(modifiers, DeclModifiers::CONSTANT) &&
                !modifiers_has(modifiers, DeclModifiers::EXTERN)) ||
               modifiers_has(modifiers, DeclModifiers::CONSTEXPR)) {
        // Constant decls must be declared with a value unless they are extern
        return make_syntax_err(syntax::Error::CONST_DECL_MISSING_VALUE, start_token);
    }

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    }
    return parser.add_stmt(start_token, DeclStatement{decl_name, decl_type, decl_value, modifiers});
}

auto DeferStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::END) ||
        parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_syntax_err(syntax::Error::DEFER_MISSING_DEFERREE, start_token);
    }
    parser.advance();
    const auto stmt = TRY(parser.parse_statement(true));

    // The statement has different restrictions from expression alternates
    if (!stmt.any<ExpressionStatement, DiscardStatement, BlockStatement>()) {
        return make_syntax_err(syntax::Error::ILLEGAL_DEFERRED_STATEMENT,
                               parser.get_location_of(*stmt));
    }
    return parser.add_stmt(start_token, DeferStatement{stmt});
}

auto DiscardStatement::parse(syntax::Parser& parser)
    -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    TRY(parser.expect_peek(syntax::TokenType::ASSIGN));
    if (parser.peek_token_is(syntax::TokenType::END) ||
        parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_syntax_err(syntax::Error::DISCARD_MISSING_DISCARDEE,
                               parser.get_current_token());
    }

    parser.advance();
    const auto expr = TRY(parser.parse_expression());

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    }
    return parser.add_stmt(start_token, DiscardStatement{expr});
}

auto ExpressionStatement::parse(syntax::Parser& parser, bool require_semicolon)
    -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    const auto expr        = TRY(parser.parse_expression());

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        if (parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
            parser.advance();
        } else if (require_semicolon) {
            TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
        }
    }
    return parser.add_stmt(start_token, ExpressionStatement{expr});
}

namespace {

[[nodiscard]] auto parse_import_payload(syntax::Parser& parser)
    -> Result<ImportPayloadHandle, syntax::Diagnostic> {
    if (parser.peek_token_is(syntax::TokenType::IDENT)) {
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        return TRY(IdentifierExpression::parse(parser));
    } else if (parser.peek_token_is(syntax::TokenType::STRING)) {
        TRY(parser.expect_peek(syntax::TokenType::STRING));
        const StringHandle string = TRY(StringExpression::parse(parser));

        if (parser.get_node<StringExpression>(*string).value.empty()) {
            return make_syntax_err(syntax::Error::EMPTY_USER_IMPORT,
                                   parser.get_location_of(*string));
        }
        return string;
    }

    return make_syntax_err(syntax::Error::ILLEGAL_IMPORT_TYPE, parser.get_peek_token());
}

} // namespace

auto ImportStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    // A start token of public is guaranteed to be followed by an import
    const auto start_token = parser.get_current_token();
    if (parser.current_token_is(syntax::TokenType::PUBLIC)) { parser.advance(); }
    auto imported_core = TRY(parse_import_payload(parser));

    opt::Option<IdentifierHandle> imported_alias;
    if (parser.peek_token_is(syntax::TokenType::AS)) {
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::IDENT));

        imported_alias.emplace(TRY(IdentifierExpression::parse(parser)));
    } else if (imported_core->get_kind() == NodeKind::STRING_EXPRESSION) {
        return make_syntax_err(syntax::Error::USER_IMPORT_MISSING_ALIAS, start_token);
    }

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    }

    return parser.add_stmt(start_token, ImportStatement{imported_core, imported_alias});
}

auto ReturnStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    opt::Option<ExpressionHandle> value;
    if (!parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        parser.advance();
        value.emplace(TRY(parser.parse_expression()));
    }

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return parser.add_stmt(start_token, ReturnStatement{value});
}

auto TestStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    opt::Option<StringHandle> description;
    if (parser.peek_token_is(syntax::TokenType::STRING)) {
        parser.advance();
        description.emplace(TRY(StringExpression::parse(parser)));

        // Empty strings aren't supported since one should just use no description
        if (parser.get_node<StringExpression>(**description).value.empty()) {
            return make_syntax_err(syntax::Error::EMPTY_TEST_DESCRIPTION,
                                   parser.get_location_of(**description));
        }
    }

    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    const BlockHandle block = TRY(BlockStatement::parse(parser));
    return parser.add_stmt(start_token, TestStatement{description, block});
}

auto UsingStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    // A start token of public is guaranteed to be followed by an import
    const auto start_token = parser.get_current_token();
    if (parser.current_token_is(syntax::TokenType::PUBLIC)) { parser.advance(); }

    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    const IdentifierHandle alias = TRY(IdentifierExpression::parse(parser));

    TRY(parser.expect_peek(syntax::TokenType::ASSIGN));
    const auto type = TRY(ExplicitType::parse(parser));

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return parser.add_stmt(start_token, UsingStatement{alias, type});
}

} // namespace porpoise::ast
