#include "ast/type.hh"

#include "syntax/parser.hh"

namespace porpoise::ast {

namespace {

auto parse_function_type(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));

    // Parse the definition now that we're at the fn token
    std::vector<FunctionExpression::Parameter> parameters;
    bool                                       variadic = false;
    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        parser.advance();
    } else {
        // There is no self parameter for types
        while (!parser.peek_token_is(syntax::TokenType::RPAREN) &&
               !parser.peek_token_is(syntax::TokenType::END)) {
            if ((variadic = TRY(FunctionExpression::try_parse_variadic(parser)))) { break; }

            // There are no default values for parameters, and they must be explicitly typed
            auto type = TRY(ExplicitType::parse(parser));
            if (type.is<IdentifierExpression>()) {
                // noreturn is not allowed for parameters
                if (type.get_token_type() == syntax::TokenType::NORETURN) {
                    return make_syntax_err(syntax::Error::FN_PARAMETER_IS_NORETURN,
                                           parser.get_location_of(type));
                }
            }

            parameters.emplace_back(opt::none, type);
            if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
                TRY(parser.expect_peek(syntax::TokenType::COMMA));
            }
        }
        TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    }

    // There must be a return type but there cannot be a block
    TRY(parser.expect_peek(syntax::TokenType::COLON));
    const auto return_type = TRY(ExplicitType::parse(parser));
    if (parser.peek_token_is(syntax::TokenType::LBRACE)) {
        return make_syntax_err(syntax::Error::EXPLICIT_FN_TYPE_HAS_BODY, start_token);
    }

    return parser.add_expr(
        start_token,
        FunctionExpression{opt::none, std::move(parameters), variadic, return_type, opt::none});
}

} // namespace

auto ExplicitType::parse(syntax::Parser& parser) -> Result<ExplicitTypeID, syntax::Diagnostic> {
    // Always check for a modifier and advance past it if present
    const auto         modifier_token = parser.get_peek_token();
    const TypeModifier modifier{modifier_token};
    if (!modifier.is_value()) { parser.advance(); }

    // The array dimension of a type are only present conditionally
    if (parser.peek_token_is(syntax::TokenType::LBRACKET)) {
        parser.advance();

        auto                          null_terminated = false;
        opt::Option<ExpressionHandle> dimension;
        if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
            parser.advance();
            null_terminated = true;
        } else if (!parser.peek_token_is(syntax::TokenType::RBRACKET)) {
            parser.advance();
            dimension.emplace(TRY(parser.parse_expression()));

            // The null terminated marker comes after the size for explicitly sized types
            if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
                parser.advance();
                null_terminated = true;
            }
        }
        TRY(parser.expect_peek(syntax::TokenType::RBRACKET));

        // Arrays are recursively defined
        const auto inner = TRY(ExplicitType::parse(parser));
        return parser.add_type(
            modifier_token, modifier, ExplicitArrayType{dimension, null_terminated, inner});
    } else if (!TypeModifier{parser.get_peek_token()}.is_value()) {
        // Don't advance since the parser does it implicitly here (costs two modifier queries)
        const auto inner = TRY(ExplicitType::parse(parser));
        return parser.add_type(modifier_token, modifier, ExplicitTypeID{inner});
    }

    // Otherwise the type has to be a 'simple' function or ident
    const auto& peek_token = parser.get_peek_token();
    if (peek_token.is_valid_ident()) {
        // It's trivial to catch these syntactic errors here
        if (!modifier.is_value()) {
            switch (peek_token.type) {
            case syntax::TokenType::TYPE_TYPE:
                return make_syntax_err(syntax::Error::ILLEGAL_TYPE_TYPE_MODIFIER, modifier_token);
            case syntax::TokenType::VOID_TYPE:
                return make_syntax_err(syntax::Error::ILLEGAL_VOID_TYPE_MODIFIER, modifier_token);
            case syntax::TokenType::NORETURN:
                return make_syntax_err(syntax::Error::ILLEGAL_NORETURN_TYPE_MODIFIER,
                                       modifier_token);
            default: break;
            }
        }

        parser.advance();
        // Manually dispatch to prevent weird consumption
        if (parser.peek_token_is(syntax::TokenType::COLON_COLON) ||
            parser.peek_token_is(syntax::TokenType::LPAREN)) {
            const auto parsed = TRY(parser.parse_expression(syntax::Precedence::TYPE));
            if (parsed->is<ScopeResolutionExpression>()) {
                return parser.add_type(
                    modifier_token,
                    modifier,
                    ScopeResolutionExpression{parser.get_node<ScopeResolutionExpression>(*parsed)});
            } else if (parsed->is<CallExpression>()) {
                return parser.add_type(modifier_token,
                                       modifier,
                                       CallExpression{parser.get_node<CallExpression>(*parsed)});
            }

            return make_syntax_err(syntax::Error::ILLEGAL_EXPLICIT_TYPE,
                                   parser.get_location_of(*parsed));
        }

        const auto ident = TRY(IdentifierExpression::parse(parser));
        return parser.add_type(modifier_token,
                               modifier,
                               IdentifierExpression{parser.get_node<IdentifierExpression>(*ident)});
    }

    // The inner type is limited to functions and user-defined types
    const auto type_start = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::FUNCTION)) {
        parser.advance();
        const auto type_expr = TRY(parse_function_type(parser));
        if (!(modifier.is_value() || modifier.is_ptr())) {
            return make_syntax_err(syntax::Error::ILLEGAL_FUNCTION_TYPE_MODIFIER, type_start);
        }

        // Function types cannot have bodies
        const auto& function = parser.get_node<FunctionExpression>(*type_expr);
        ASSERT(!function.body, "Function type has body");
        return parser.add_type(modifier_token, modifier, FunctionExpression{function});
    }

    // The user-defined types can be handled by parsing any expression and verifying it
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::END)) {
        return make_syntax_err(syntax::Error::MISSING_EXPLICIT_TYPE, type_start);
    }

    switch (const auto user = TRY(parser.parse_expression()); user->get_kind()) {
    case NodeKind::STRUCT_EXPRESSION:
        return parser.add_type(
            modifier_token, modifier, StructExpression{parser.get_node<StructExpression>(*user)});
    case NodeKind::ENUM_EXPRESSION:
        return parser.add_type(
            modifier_token, modifier, EnumExpression{parser.get_node<EnumExpression>(*user)});
    case NodeKind::UNION_EXPRESSION:
        return parser.add_type(
            modifier_token, modifier, UnionExpression{parser.get_node<UnionExpression>(*user)});
    default: break;
    }
    return make_syntax_err(syntax::Error::ILLEGAL_EXPLICIT_TYPE, type_start);
}

namespace {

// Parses the explicit type if present and checks for an upcoming assignment for init
[[nodiscard]] auto parse_type_and_initializer(syntax::Parser& parser)
    -> Result<std::pair<TypeHandle, bool>, syntax::Diagnostic> {
    // The start start token is always offset as this is called irregularly
    const auto start_token = parser.get_peek_token();

    if (parser.peek_token_is(syntax::TokenType::WALRUS)) {
        const auto type_expr = parser.add_node<TypeHandle>(start_token, TypeExpression{opt::none});
        parser.advance();
        return std::pair{type_expr, true};
    } else if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance();
        const auto explicit_type = TRY(ExplicitType::parse(parser));
        const auto type_expr =
            parser.add_node<TypeHandle>(start_token, TypeExpression{explicit_type});
        if (parser.peek_token_is(syntax::TokenType::ASSIGN)) {
            parser.advance();
            return std::pair{type_expr, true};
        }
        return std::pair{type_expr, false};
    }

    return Err{parser.peek_error(syntax::TokenType::COLON)};
}

} // namespace

auto TypeExpression::parse(syntax::Parser& parser)
    -> Result<std::pair<TypeHandle, bool>, syntax::Diagnostic> {
    const auto [type, initialized] = TRY(parse_type_and_initializer(parser));

    // Advance again to prepare for rhs
    if (initialized) { parser.advance(); }
    return std::pair{type, initialized};
}

} // namespace porpoise::ast
