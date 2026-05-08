#include "ast/nodes.hh"

#include "ast/ast.hh"
#include "ast/id.hh"

namespace porpoise::ast {

namespace {

// Returns an actual value only if a terminal condition was found
[[nodiscard]] auto validate_common(const DeclStatement& decl) noexcept -> opt::Option<bool> {
    // Members that violate this wouldn't be usable with C
    if (decl.has_modifier(DeclModifiers::EXTERN) || decl.has_modifier(DeclModifiers::EXPORT)) {
        return false;
    }

    if (decl.value && (*decl.value)->is<FunctionExpression>()) { return true; }
    return opt::none;
}

} // namespace

auto Members::validate_struct_decl(const DeclStatement& decl) noexcept -> bool {
    // A non-static member must always be a variable to simplify the mental model
    if (const auto result = validate_common(decl)) { return *result; }
    if (!decl.has_modifier(DeclModifiers::STATIC)) {
        return decl.has_modifier(DeclModifiers::VARIABLE);
    }
    return true;
}

auto Members::validate_non_struct_decl(const DeclStatement& decl) noexcept -> bool {
    if (const auto result = validate_common(decl)) { return *result; }
    return decl.has_modifier(DeclModifiers::STATIC);
}

auto ArrayExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    auto                          null_terminated = false;
    opt::Option<ExpressionHandle> size;
    if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
        parser.advance();
        null_terminated = true;
    } else if (!parser.peek_token_is(syntax::TokenType::RBRACKET)) {
        parser.advance();
        if (parser.current_token_is(syntax::TokenType::RBRACKET)) {
            return make_syntax_err(syntax::Error::MISSING_ARRAY_SIZE_TOKEN, start_token);
        } else if (!parser.current_token_is(syntax::TokenType::UNDERSCORE)) {
            size.emplace(TRY(parser.parse_expression()));
        }

        // The null terminated marker comes after the size for explicitly sized types
        if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
            parser.advance();
            null_terminated = true;
        }
    } else {
        // There needs to be a token for the size for array literals
        return make_syntax_err(syntax::Error::MISSING_ARRAY_SIZE_TOKEN, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::RBRACKET));
    auto item_type = TRY(ExplicitType::parse(parser));
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    // Current token is either the LBRACE at the start or a comma before parsing
    std::vector<ExpressionHandle> items;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        items.emplace_back(TRY(parser.parse_expression()));
        if (!parser.peek_token_is(syntax::TokenType::RBRACE)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }

    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    return ExpressionHandle{parser.get_ast().add_node(
        start_token, ArrayExpression{size, null_terminated, item_type, std::move(items)})};
}

auto CallExpression::parse(syntax::Parser& parser, ExpressionHandle function)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto            start_token = parser.get_current_token();
    std::vector<Argument> arguments;
    // Guaranteed to roll back if there is an error
    const auto parse_expr_unsuccessful = [&] {
        // Try an expression first to prevent ambiguity between reference operators
        syntax::Parser::Transaction transaction{parser};
        parser.advance();
        if (auto expr = parser.parse_expression()) {
            transaction.commit();
            arguments.emplace_back(std::move(*expr));
            return false;
        }
        return true;
    };

    while (!parser.peek_token_is(syntax::TokenType::RPAREN) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        if (parser.peek_token_is(syntax::TokenType::COMMA)) {
            return make_syntax_err(syntax::Error::COMMA_WITH_MISSING_CALL_ARGUMENT,
                                   parser.get_peek_token());
        }

        // Advance cannot be called here since explicit type relies on peek, not current
        if (parse_expr_unsuccessful()) { arguments.emplace_back(TRY(ExplicitType::parse(parser))); }
        if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    return ExpressionHandle{
        parser.get_ast().add_node(start_token, CallExpression{function, std::move(arguments)})};
}

namespace {

[[nodiscard]] constexpr auto block_is_empty(syntax::Parser& parser, BlockHandle block) -> bool {
    return std::get<BlockStatement>(parser.get_ast()[*block]).empty();
}

} // namespace

auto DoWhileLoopExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    const BlockHandle block = TRY(BlockStatement::parse(parser));
    TRY(parser.expect_peek(syntax::TokenType::WHILE));
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));

    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        return make_syntax_err(syntax::Error::WHILE_MISSING_CONDITION, parser.get_current_token());
    }
    parser.advance();

    // There's no continuation or non break clause so this is easy :)
    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    if (block_is_empty(parser, block)) {
        return make_syntax_err(syntax::Error::EMPTY_LOOP, parser.get_location_of(*block));
    }

    return ExpressionHandle{
        parser.get_ast().add_node(start_token, DoWhileLoopExpression{block, condition})};
}

auto EnumExpression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    opt::Option<IdentifierHandle> underlying;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance(2);
        underlying.emplace(TRY(IdentifierExpression::parse(parser)));
    }
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    std::vector<Enumeration> enumerations;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        if (parser.get_peek_token().is_member_token()) { break; }

        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        const IdentifierHandle ident = TRY(IdentifierExpression::parse(parser));

        opt::Option<ExpressionHandle> value;
        if (parser.peek_token_is(syntax::TokenType::ASSIGN)) {
            parser.advance(2);
            value.emplace(TRY(parser.parse_expression()));
        }
        enumerations.emplace_back(std::move(ident), std::move(value));

        // No comma means that its the end or that there is a decl list starting
        if (!parser.peek_token_is(syntax::TokenType::COMMA)) { break; }
        parser.advance();
    }

    auto members = TRY(Members::parse(parser, [&](const Members::Member& member) {
        return std::visit(Overloaded{[](const DeclStatement& decl) {
                                         return Members::validate_non_struct_decl(decl);
                                     },
                                     [](const auto&) { return true; }},
                          parser.get_ast()[*member]);
    }));
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    // Validate here so that there aren't 3 errors spawning from an empty enum with decls
    if (enumerations.empty()) { return make_syntax_err(syntax::Error::EMPTY_ENUM, start_token); }
    return ExpressionHandle{parser.get_ast().add_node(
        start_token, EnumExpression{underlying, std::move(enumerations), std::move(members)})};
}

auto ForLoopExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    // Iterables have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        return make_syntax_err(syntax::Error::FOR_MISSING_ITERABLES, start_token);
    }

    std::vector<ExpressionHandle> iterables;
    while (!parser.peek_token_is(syntax::TokenType::RPAREN) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();

        auto iterable = TRY(parser.parse_expression());
        iterables.emplace_back(std::move(iterable));

        if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }

    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    // Captures take on something similar to zig's capture syntax
    std::vector<Capture> captures;
    TRY(parser.expect_peek(syntax::TokenType::BW_OR));
    while (!parser.peek_token_is(syntax::TokenType::BW_OR) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        if (parser.current_token_is(syntax::TokenType::UNDERSCORE)) {
            const auto discarded =
                parser.get_ast().add_node(parser.get_current_token(), Discarded{});
            captures.emplace_back(type_modifiers::VALUE, Capture::PayloadHandle{discarded});
        } else {
            // Always check for a modifier and advance past it if present
            const TypeModifier modifier{parser.get_current_token()};
            if (!modifier.is_value()) { parser.advance(); }

            IdentifierHandle capture = TRY(IdentifierExpression::parse(parser));
            captures.emplace_back(modifier, capture);
        }

        if (!parser.peek_token_is(syntax::TokenType::BW_OR)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    TRY(parser.expect_peek(syntax::TokenType::BW_OR));

    // Loops must have a well formed block and may have an alternate in non-break cases
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    BlockHandle block = TRY(BlockStatement::parse(parser));
    auto        non_break =
        TRY(parser.try_parse_restricted_alternate(syntax::Error::ILLEGAL_LOOP_NON_BREAK));

    // The number of captures must align with the number of iterables
    if (captures.size() != iterables.size()) {
        return make_syntax_err(syntax::Error::FOR_ITERABLE_CAPTURE_MISMATCH, start_token);
    }

    if (block_is_empty(parser, block)) {
        return make_syntax_err(syntax::Error::EMPTY_LOOP, parser.get_location_of(*block));
    }

    return ExpressionHandle{parser.get_ast().add_node(
        start_token,
        ForLoopExpression{
            std::move(iterables), std::move(captures), std::move(block), std::move(non_break)})};
}

namespace {

// Variadic must be handled first and should break the enclosing loop
[[nodiscard]] auto try_parse_variadic(syntax::Parser& parser) -> Result<bool, syntax::Diagnostic> {
    bool variadic = false;
    if (parser.peek_token_is(syntax::TokenType::ELLIPSIS)) {
        parser.advance();
        variadic = true;
        if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    return variadic;
}

} // namespace

auto FunctionExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));

    // Parse the definition now that we're at the fn token
    opt::Option<SelfParameter> self;
    std::vector<Parameter>     parameters;
    bool                       variadic = false;
    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        parser.advance();
    } else if ((variadic = TRY(try_parse_variadic(parser)))) {
        TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    } else {
        // The 'self' parameter can be a value type, ref, or mutable ref
        parser.advance();
        const TypeModifier self_modifier{parser.get_current_token()};
        if (self_modifier.is_value() && (parser.peek_token_is(syntax::TokenType::COMMA) ||
                                         parser.peek_token_is(syntax::TokenType::RPAREN))) {
            const IdentifierHandle ident = TRY(IdentifierExpression::parse(parser));
            self.emplace(SelfParameter{self_modifier, ident});

            // Still end on a comma
            if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
                TRY(parser.expect_peek(syntax::TokenType::COMMA));
            }
        } else if (!self_modifier.is_value() && parser.peek_token_is(syntax::TokenType::IDENT)) {
            // Move up to the ident before parsing it
            parser.advance();
            const IdentifierHandle ident = TRY(IdentifierExpression::parse(parser));
            self.emplace(SelfParameter{self_modifier, ident});

            // Move to the comma if present
            if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
                TRY(parser.expect_peek(syntax::TokenType::COMMA));
            }
        }

        // The loop starts either on an LPAREN or COMMA
        bool first = true;
        while (!parser.peek_token_is(syntax::TokenType::RPAREN) &&
               !parser.peek_token_is(syntax::TokenType::END)) {
            if ((variadic = TRY(try_parse_variadic(parser)))) { break; }

            // If there was no self parameter then we can't advance on the first pass
            if (!first || self) { parser.advance(); }
            IdentifierHandle name         = TRY(IdentifierExpression::parse(parser));
            auto [type_expr, initialized] = TRY(TypeExpression::parse(parser));
            auto type                     = std::get<TypeExpression>(parser.get_ast()[*type_expr]);

            // There are no default values for parameters, and they must be explicitly typed
            if (initialized || !type.explicit_type) {
                return make_syntax_err(syntax::Error::FN_PARAMETER_HAS_DEFAULT_VALUE,
                                       parser.get_location_of(*type_expr));
            }

            const auto& explicit_type = *type.explicit_type;
            if (explicit_type.is<IdentifierExpression>()) {
                // noreturn is not allowed for parameters
                if (explicit_type.get_token_type() == syntax::TokenType::NORETURN) {
                    return make_syntax_err(syntax::Error::FN_PARAMETER_IS_NORETURN,
                                           parser.get_location_of(*type_expr));
                }
            }

            parameters.emplace_back(name, explicit_type);
            if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
                TRY(parser.expect_peek(syntax::TokenType::COMMA));
            }
            first = false;
        }
        TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    }

    TRY(parser.expect_peek(syntax::TokenType::COLON));
    auto return_type = TRY(ExplicitType::parse(parser));

    // If there is opening brace then just return without a body
    if (!parser.peek_token_is(syntax::TokenType::LBRACE)) {
        return make_syntax_err(syntax::Error::FN_DECLARATION_WITHOUT_BODY, start_token);
    }

    // Otherwise there must be a well-formed block
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    BlockHandle body = TRY(BlockStatement::parse(parser));
    return ExpressionHandle{parser.get_ast().add_node(
        start_token, FunctionExpression{self, std::move(parameters), variadic, return_type, body})};
}

auto FunctionExpression::parse_type(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));

    // Parse the definition now that we're at the fn token
    std::vector<Parameter> parameters;
    bool                   variadic = false;
    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        parser.advance();
    } else {
        // There is no self parameter for types
        while (!parser.peek_token_is(syntax::TokenType::RPAREN) &&
               !parser.peek_token_is(syntax::TokenType::END)) {
            if ((variadic = TRY(try_parse_variadic(parser)))) { break; }

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
    auto return_type = TRY(ExplicitType::parse(parser));
    if (parser.peek_token_is(syntax::TokenType::LBRACE)) {
        return make_syntax_err(syntax::Error::EXPLICIT_FN_TYPE_HAS_BODY, start_token);
    }

    return ExpressionHandle{parser.get_ast().add_node(
        start_token,
        FunctionExpression{opt::none, std::move(parameters), variadic, return_type, opt::none})};
}

auto GroupedExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    parser.advance();
    auto inner = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    return inner;
}

auto IdentifierExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    if (!start_token.is_valid_ident()) {
        return make_syntax_err(syntax::Error::ILLEGAL_IDENTIFIER, start_token);
    }

    return ExpressionHandle{
        parser.get_ast().add_node(start_token, IdentifierExpression{start_token.slice})};
}

auto IfExpression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto IndexExpression::parse(syntax::Parser& parser, ExpressionHandle array)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser, array);
}

auto InfiniteLoopExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto AssignmentExpression::parse(syntax::Parser& parser, ExpressionHandle lhs)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser, lhs);
}

auto BinaryExpression::parse(syntax::Parser& parser, ExpressionHandle lhs)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser, lhs);
}

auto DotExpression::parse(syntax::Parser& parser, ExpressionHandle lhs)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser, lhs);
}

auto RangeExpression::parse(syntax::Parser& parser, ExpressionHandle lhs)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser, lhs);
}

auto InitializerExpression::parse(syntax::Parser& parser, opt::Option<ExpressionHandle> object)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser, object);
}

auto LabelExpression::parse(syntax::Parser& parser, ExpressionHandle name)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser, name);
}

auto LabelExpression::deconstruct_body(StatementHandle raw_stmt)
    -> Result<LabeledNodeHandle, syntax::Diagnostic> {
    TODO(raw_stmt);
}

auto MatchExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto UnaryExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto ReferenceExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto DereferenceExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto ImplicitAccessExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto StringExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto I32Expression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto I64Expression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto ISizeExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto U32Expression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto U64Expression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto USizeExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto U8Expression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto F32Expression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto F64Expression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto BoolExpression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto VoidExpression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto ScopeResolutionExpression::parse(syntax::Parser& parser, ExpressionHandle outer)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser, outer);
}

auto StructExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto ExplicitType::parse(syntax::Parser& parser) -> Result<ExplicitTypeID, syntax::Diagnostic> {
    TODO(parser);
}

auto TypeExpression::parse(syntax::Parser& parser)
    -> Result<std::pair<TypeHandle, bool>, syntax::Diagnostic> {
    TODO(parser);
}

auto UnionExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto WhileLoopExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto BlockStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto BreakStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto ContinueStatement::parse(syntax::Parser& parser)
    -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto DeclStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto DeferStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto DiscardStatement::parse(syntax::Parser& parser)
    -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto ExpressionStatement::parse(syntax::Parser& parser, bool require_semicolon)
    -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser, require_semicolon);
}

auto ImportStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto ReturnStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto TestStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

auto UsingStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    TODO(parser);
}

} // namespace porpoise::ast
