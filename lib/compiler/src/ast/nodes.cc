#include "ast/nodes.hh"

#include "ast/ast.hh"
#include "ast/id.hh"

#include "enum.hh"

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
    const auto start_token = parser.get_current_token();

    bool constexpr_condition = false;
    if (parser.peek_token_is(syntax::TokenType::CONSTEXPR)) {
        constexpr_condition = true;
        parser.advance();
    }

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::RPAREN)) {
        return make_syntax_err(syntax::Error::IF_MISSING_CONDITION, start_token);
    }

    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    // The consequence and alternate are trivially handled by restricted statement parsers
    parser.advance();
    auto consequence =
        TRY(parser.parse_restricted_statement(syntax::Error::ILLEGAL_IF_BRANCH, false));
    auto alternate =
        TRY(parser.try_parse_restricted_alternate(syntax::Error::ILLEGAL_IF_BRANCH, false));

    return ExpressionHandle{parser.get_ast().add_node(
        start_token, IfExpression{constexpr_condition, condition, consequence, alternate})};
}

auto IndexExpression::parse(syntax::Parser& parser, ExpressionHandle array)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::RBRACKET)) {
        return make_syntax_err(syntax::Error::INDEX_MISSING_EXPRESSION, start_token);
    }
    parser.advance();

    auto idx_expr = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RBRACKET));
    return ExpressionHandle{
        parser.get_ast().add_node(start_token, IndexExpression{array, idx_expr})};
}

auto InfiniteLoopExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    const BlockHandle block = TRY(BlockStatement::parse(parser));
    if (block_is_empty(parser, block)) {
        return make_syntax_err(syntax::Error::EMPTY_LOOP, parser.get_location_of(*block));
    }
    return ExpressionHandle{parser.get_ast().add_node(start_token, InfiniteLoopExpression{block})};
}

namespace {

template <traits::ASTNode Node>
[[nodiscard]] auto parse_infix(syntax::Parser& parser, ExpressionHandle lhs)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto op_token = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::END)) {
        return make_syntax_err(syntax::Error::INFIX_MISSING_RHS, op_token);
    }

    auto [current_precedence, current_binding] = parser.get_current_precedence();
    if (current_binding && current_binding->right_assoc) {
        current_precedence =
            static_cast<syntax::Precedence>(std::to_underlying(current_precedence) - 1);
    }

    parser.advance();
    auto rhs = TRY(parser.parse_expression(current_precedence));
    return ExpressionHandle{parser.get_ast().add_node(op_token, Node{lhs, rhs})};
}

} // namespace

#define MAKE_INFIX_PARSER(Type)                                    \
    auto Type::parse(syntax::Parser& parser, ExpressionHandle lhs) \
        -> Result<ExpressionHandle, syntax::Diagnostic> {          \
        return parse_infix<Type>(parser, lhs);                     \
    }

MAKE_INFIX_PARSER(AssignmentExpression)
MAKE_INFIX_PARSER(BinaryExpression)
MAKE_INFIX_PARSER(DotExpression)
MAKE_INFIX_PARSER(RangeExpression)

auto InitializerExpression::parse(syntax::Parser& parser, opt::Option<ExpressionHandle> object)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    std::vector<Initializer> initializers;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        TRY(parser.expect_peek(syntax::TokenType::DOT));
        ImplicitAccessHandle member = TRY(ImplicitAccessExpression::parse(parser));

        TRY(parser.expect_peek(syntax::TokenType::ASSIGN));
        parser.advance();
        auto value = TRY(parser.parse_expression());
        initializers.emplace_back(member, value);

        if (!parser.peek_token_is(syntax::TokenType::RBRACE)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    return ExpressionHandle{parser.get_ast().add_node(
        start_token, InitializerExpression{object, std::move(initializers)})};
}

auto LabelExpression::parse(syntax::Parser& parser, ExpressionHandle name)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto& start_token = parser.get_current_token();
    if (!name->is<IdentifierExpression>()) {
        return make_syntax_err(syntax::Error::ILLEGAL_LABEL, start_token);
    }
    parser.advance();

    // The body has to be constructed in place from a lambda as it cannot be default initialized
    const auto raw_stmt = TRY(parser.parse_statement(true));
    const auto body     = TRY(deconstruct_body(parser, raw_stmt));

    IdentifierHandle name_handle = name;
    return ExpressionHandle{parser.get_ast().add_node(start_token, LabelExpression{name, body})};
}

auto LabelExpression::deconstruct_body(syntax::Parser& parser, StatementHandle raw_stmt)
    -> Result<LabeledNodeHandle, syntax::Diagnostic> {
    switch (raw_stmt->get_kind()) {
    case NodeKind::EXPRESSION_STATEMENT: {
        const auto& expr_stmt = std::get<ExpressionStatement>(parser.get_ast()[*raw_stmt]);
        switch (expr_stmt.expression->get_kind()) {
        case NodeKind::DO_WHILE_LOOP_EXPRESSION:
        case NodeKind::FOR_LOOP_EXPRESSION:
        case NodeKind::IF_EXPRESSION:
        case NodeKind::INFINITE_LOOP_EXPRESSION:
        case NodeKind::MATCH_EXPRESSION:
        case NodeKind::WHILE_LOOP_EXPRESSION:    return expr_stmt.expression;
        default:
            return make_syntax_err(syntax::Error::ILLEGAL_LABEL_EXPRESSION,
                                   parser.get_location_of(*raw_stmt));
        }
    }
    case NodeKind::BLOCK_STATEMENT: return raw_stmt;
    default:
        return make_syntax_err(syntax::Error::ILLEGAL_LABEL_STATEMENT,
                               parser.get_location_of(*raw_stmt));
    }
}

auto MatchExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::RPAREN)) {
        return make_syntax_err(syntax::Error::MATCH_EXPR_MISSING_CONDITION, start_token);
    }

    auto matcher = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    if (parser.peek_token_is(syntax::TokenType::RBRACE)) {
        parser.advance();
        return make_syntax_err(syntax::Error::ARMLESS_MATCH_EXPR, start_token);
    }

    std::vector<Arm> arms;
    // Current token is either the LBRACE at the start or a comma before parsing
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();

        auto pattern = TRY(parser.parse_expression());
        TRY(parser.expect_peek(syntax::TokenType::FAT_ARROW));

        // There is an optional capture for every arm
        opt::Option<Arm::CaptureHandle> capture;
        if (parser.peek_token_is(syntax::TokenType::BW_OR)) {
            parser.advance();

            // An underscore is equivalent to a lack of capture
            if (parser.peek_token_is(syntax::TokenType::UNDERSCORE)) {
                parser.advance();
                capture.emplace(Arm::CaptureHandle{
                    parser.get_ast().add_node(parser.get_current_token(), Discarded{})});
            } else {
                TRY(parser.expect_peek(syntax::TokenType::IDENT));
                capture.emplace(TRY(IdentifierExpression::parse(parser)));
            }
            TRY(parser.expect_peek(syntax::TokenType::BW_OR));
        }

        // The resulting statement must be restricted like an if branch
        parser.advance();
        auto consequence =
            TRY(parser.parse_restricted_statement(syntax::Error::ILLEGAL_MATCH_ARM, false));
        arms.emplace_back(pattern, capture, consequence);
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    auto catch_all =
        TRY(parser.try_parse_restricted_alternate(syntax::Error::ILLEGAL_MATCH_CATCH_ALL));

    return ExpressionHandle{parser.get_ast().add_node(
        start_token, MatchExpression{matcher, std::move(arms), catch_all})};
}

namespace {

template <traits::ASTNode Node>
[[nodiscard]] auto parse_prefix(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto prefix_token = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::END)) {
        return make_syntax_err(syntax::Error::PREFIX_MISSING_OPERAND, prefix_token);
    }
    parser.advance();

    auto operand = TRY(parser.parse_expression(syntax::Precedence::PREFIX));
    return ExpressionHandle{parser.get_ast().add_node(prefix_token, Node{operand})};
}

} // namespace

#define MAKE_PREFIX_PARSER(Type)                                                               \
    auto Type::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> { \
        return parse_prefix<Type>(parser);                                                     \
    }

MAKE_PREFIX_PARSER(UnaryExpression)
MAKE_PREFIX_PARSER(ReferenceExpression)
MAKE_PREFIX_PARSER(DereferenceExpression)

auto ImplicitAccessExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    // We need to explicitly jump into the initializer expression here
    if (parser.peek_token_is(syntax::TokenType::LBRACE)) {
        parser.advance();
        return InitializerExpression::parse(parser, opt::none);
    }

    // Otherwise it suffices to fall back to standard prefix parsing
    const auto prefix_token = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::END)) {
        return make_syntax_err(syntax::Error::PREFIX_MISSING_OPERAND, prefix_token);
    }

    parser.advance();
    auto operand = TRY(parser.parse_expression(syntax::Precedence::PREFIX));
    if (!operand->is<IdentifierExpression>()) {
        return make_syntax_err(syntax::Error::ILLEGAL_IMPLICIT_ACCESS_OPERAND,
                               parser.get_location_of(*operand));
    }

    return ExpressionHandle{
        parser.get_ast().add_node(prefix_token, ImplicitAccessExpression{operand})};
}

auto StringExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    return ExpressionHandle{
        parser.get_ast().add_node(start_token, StringExpression{start_token.materialize_string()})};
}

namespace {

template <traits::ASTNode Node>
static auto parse_primitive(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    using value_type       = typename Node::value_type;
    const auto start_token = parser.get_current_token();
    auto value = detail::parse_primitive_value<value_type>(start_token.slice, start_token.type);
    if (value) { return ExpressionHandle{parser.get_ast().add_node(start_token, Node{*value})}; }

    return make_syntax_err(std::is_same_v<value_type, f64>
                               ? syntax::Error::DOUBLE_OVERFLOW
                               : (std::is_same_v<value_type, f32>
                                      ? syntax::Error::FLOAT_OVERFLOW
                                      : syntax::Error::INTEGER_OVERFLOW),
                           start_token);
}

} // namespace

#define MAKE_PRIMITIVE_PARSER(Type)                                                            \
    auto Type::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> { \
        return parse_primitive<Type>(parser);                                                  \
    }

MAKE_PRIMITIVE_PARSER(I32Expression)
MAKE_PRIMITIVE_PARSER(I64Expression)
MAKE_PRIMITIVE_PARSER(ISizeExpression)
MAKE_PRIMITIVE_PARSER(U32Expression)
MAKE_PRIMITIVE_PARSER(U64Expression)
MAKE_PRIMITIVE_PARSER(USizeExpression)

auto U8Expression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    const auto slice       = start_token.slice;
    if (slice[1] != '\\') {
        return ExpressionHandle{
            parser.get_ast().add_node(start_token, U8Expression{static_cast<u8>(slice[1])})};
    }

    const auto escaped = slice[2];
    u8         value;
    switch (escaped) {
    case 'n':  value = '\n'; break;
    case 'r':  value = '\r'; break;
    case 't':  value = '\t'; break;
    case '\\': value = '\\'; break;
    case '\'': value = '\''; break;
    case '"':  value = '"'; break;
    case '0':  value = '\0'; break;
    default:   return make_syntax_err(syntax::Error::UNKNOWN_CHARACTER_ESCAPE, start_token);
    }

    return ExpressionHandle{parser.get_ast().add_node(start_token, U8Expression{value})};
}

MAKE_PRIMITIVE_PARSER(F32Expression)
MAKE_PRIMITIVE_PARSER(F64Expression)

auto BoolExpression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    return ExpressionHandle{
        parser.get_ast().add_node(parser.get_current_token(), BoolExpression{})};
}

auto VoidExpression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    return ExpressionHandle{parser.get_ast().add_node(start_token, VoidExpression{})};
}

auto ScopeResolutionExpression::parse(syntax::Parser& parser, ExpressionHandle outer)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    if (!outer->any<IdentifierExpression, ScopeResolutionExpression>()) {
        return make_syntax_err(syntax::Error::ILLEGAL_OUTER_SCOPE_TYPE,
                               parser.get_location_of(*outer));
    }

    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    const IdentifierHandle inner = TRY(IdentifierExpression::parse(parser));
    return ExpressionHandle{
        parser.get_ast().add_node(start_token, ScopeResolutionExpression{outer, inner})};
}

auto StructExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    auto members = TRY(Members::parse(parser, [&](const Members::Member& member) {
        return std::visit(Overloaded{[](const DeclStatement& decl) {
                                         return Members::validate_struct_decl(decl);
                                     },
                                     [](const auto&) { return true; }},
                          parser.get_ast()[*member]);
    }));

    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    return ExpressionHandle{
        parser.get_ast().add_node(start_token, StructExpression{std::move(members)})};
}

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
        auto inner = TRY(ExplicitType::parse(parser));
        return parser.get_ast().add_type(
            modifier_token, modifier, ExplicitArrayType{dimension, null_terminated, inner});
    } else if (!TypeModifier{parser.get_peek_token()}.is_value()) {
        // Don't advance since the parser does it implicitly here (costs two modifier queries)
        auto inner = TRY(ExplicitType::parse(parser));
        return parser.get_ast().add_type(modifier_token, modifier, ExplicitTypeID{inner});
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
            auto parsed = TRY(parser.parse_expression(syntax::Precedence::TYPE));
            if (parsed->is<ScopeResolutionExpression>()) {
                return parser.get_ast().add_type(
                    modifier_token,
                    modifier,
                    ScopeResolutionExpression{
                        std::get<ScopeResolutionExpression>(parser.get_ast()[*parsed])});
            } else if (parsed->is<CallExpression>()) {
                return parser.get_ast().add_type(
                    modifier_token,
                    modifier,
                    CallExpression{std::get<CallExpression>(parser.get_ast()[*parsed])});
            }

            return make_syntax_err(syntax::Error::ILLEGAL_EXPLICIT_TYPE,
                                   parser.get_location_of(*parsed));
        }

        const auto ident = TRY(IdentifierExpression::parse(parser));
        return parser.get_ast().add_type(
            modifier_token,
            modifier,
            IdentifierExpression{std::get<IdentifierExpression>(parser.get_ast()[*ident])});
    }

    // The inner type is limited to functions and user-defined types
    const auto type_start = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::FUNCTION)) {
        parser.advance();
        auto type_expr = TRY(FunctionExpression::parse_type(parser));
        if (!(modifier.is_value() || modifier.is_ptr())) {
            return make_syntax_err(syntax::Error::ILLEGAL_FUNCTION_TYPE_MODIFIER, type_start);
        }

        // Function types cannot have bodies
        const auto& function = std::get<FunctionExpression>(parser.get_ast()[*type_expr]);
        ASSERT(!function.body, "Function type has body");
        return parser.get_ast().add_type(modifier_token, modifier, FunctionExpression{function});
    }

    // The user-defined types can be handled by parsing any expression and verifying it
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::END)) {
        return make_syntax_err(syntax::Error::MISSING_EXPLICIT_TYPE, type_start);
    }

    switch (auto user = TRY(parser.parse_expression()); user->get_kind()) {
    case NodeKind::STRUCT_EXPRESSION:
        return parser.get_ast().add_type(
            modifier_token,
            modifier,
            StructExpression{std::get<StructExpression>(parser.get_ast()[*user])});
    case NodeKind::ENUM_EXPRESSION:
        return parser.get_ast().add_type(
            modifier_token,
            modifier,
            EnumExpression{std::get<EnumExpression>(parser.get_ast()[*user])});
    case NodeKind::UNION_EXPRESSION:
        return parser.get_ast().add_type(
            modifier_token,
            modifier,
            UnionExpression{std::get<UnionExpression>(parser.get_ast()[*user])});
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
        auto type_expr =
            TypeHandle{parser.get_ast().add_node(start_token, TypeExpression{opt::none})};
        parser.advance();
        return std::pair{type_expr, true};
    } else if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance();
        auto explicit_type = TRY(ExplicitType::parse(parser));
        auto type_expr =
            TypeHandle{parser.get_ast().add_node(start_token, TypeExpression{explicit_type})};
        if (parser.peek_token_is(syntax::TokenType::ASSIGN)) {
            parser.advance();
            return std::pair{std::move(type_expr), true};
        }
        return std::pair{std::move(type_expr), false};
    }

    return Err{parser.peek_error(syntax::TokenType::COLON)};
}

} // namespace

auto TypeExpression::parse(syntax::Parser& parser)
    -> Result<std::pair<TypeHandle, bool>, syntax::Diagnostic> {
    auto [type, initialized] = TRY(parse_type_and_initializer(parser));

    // Advance again to prepare for rhs
    if (initialized) { parser.advance(); }
    return std::pair{type, initialized};
}

auto UnionExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    std::vector<Field> fields;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        if (parser.get_peek_token().is_member_token()) { break; }

        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        IdentifierHandle ident = TRY(IdentifierExpression::parse(parser));

        TRY(parser.expect_peek(syntax::TokenType::COLON));
        auto type = TRY(ExplicitType::parse(parser));

        fields.emplace_back(std::move(ident), std::move(type));

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

    // Validate here so that there aren't 3 errors spawning from an empty union with decls
    if (fields.empty()) { return make_syntax_err(syntax::Error::EMPTY_UNION, start_token); }
    return ExpressionHandle{parser.get_ast().add_node(
        start_token, UnionExpression{std::move(fields), std::move(members)})};
}

auto WhileLoopExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::RPAREN)) {
        return make_syntax_err(syntax::Error::WHILE_MISSING_CONDITION, start_token);
    }

    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    // Continuation expression is optional and is handled as in zig
    opt::Option<ExpressionHandle> continuation;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        const auto continuation_start = parser.get_current_token();
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::LPAREN));

        // Consume again to look at the actual expr start
        parser.advance();
        if (parser.current_token_is(syntax::TokenType::RPAREN)) {
            return make_syntax_err(syntax::Error::EMPTY_WHILE_CONTINUATION, continuation_start);
        }

        continuation.emplace(TRY(parser.parse_expression()));
        TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    }

    // Loops must have a well formed block and may have an alternate in non-break cases
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    const BlockHandle block = TRY(BlockStatement::parse(parser));
    auto              non_break =
        TRY(parser.try_parse_restricted_alternate(syntax::Error::ILLEGAL_LOOP_NON_BREAK));

    // There needs to be at least a continuation or block
    if (!continuation && block_is_empty(parser, block)) {
        return make_syntax_err(syntax::Error::EMPTY_LOOP, parser.get_location_of(*block));
    }

    return ExpressionHandle{parser.get_ast().add_node(
        start_token, WhileLoopExpression{condition, continuation, block, non_break})};
}

auto BlockStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    Statements statements;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        statements.emplace_back(TRY(parser.parse_statement(true)));
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    return StatementHandle{
        parser.get_ast().add_node(start_token, BlockStatement{std::move(statements)})};
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
    return StatementHandle{parser.get_ast().add_node(start_token, BreakStatement{label, value})};
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
    return StatementHandle{parser.get_ast().add_node(start_token, ContinueStatement{label})};
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
            return make_syntax_err(syntax::Error::DUPLICATE_DECL_MODIFIER, start_token);
        }
        modifiers |= *current_modifier;
    }

    if (!validate_modifiers(modifiers)) {
        return make_syntax_err(syntax::Error::ILLEGAL_DECL_MODIFIERS, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    IdentifierHandle decl_name          = TRY(IdentifierExpression::parse(parser));
    auto [decl_type, value_initialized] = TRY(TypeExpression::parse(parser));

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
    return StatementHandle{parser.get_ast().add_node(
        start_token, DeclStatement{decl_name, decl_type, decl_value, modifiers})};
}

auto DeferStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::END) ||
        parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_syntax_err(syntax::Error::DEFER_MISSING_DEFERREE, start_token);
    }
    parser.advance();
    auto stmt = TRY(parser.parse_statement(true));

    // The statement has different restrictions from expression alternates
    if (!stmt->any<ExpressionStatement, DiscardStatement, BlockStatement>()) {
        return make_syntax_err(syntax::Error::ILLEGAL_DEFERRED_STATEMENT,
                               parser.get_location_of(*stmt));
    }
    return StatementHandle{parser.get_ast().add_node(start_token, DeferStatement{stmt})};
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
    auto expr = TRY(parser.parse_expression());

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    }
    return StatementHandle{parser.get_ast().add_node(start_token, DiscardStatement{expr})};
}

auto ExpressionStatement::parse(syntax::Parser& parser, bool require_semicolon)
    -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    auto       expr        = TRY(parser.parse_expression());

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        if (parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
            parser.advance();
        } else if (require_semicolon) {
            TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
        }
    }
    return StatementHandle{parser.get_ast().add_node(start_token, ExpressionStatement{expr})};
}

namespace {

[[nodiscard]] constexpr auto string_is_empty(syntax::Parser& parser, StringHandle string) -> bool {
    return std::get<StringExpression>(parser.get_ast()[*string]).value.empty();
}

[[nodiscard]] auto parse_import_core(syntax::Parser& parser)
    -> Result<ImportStatement::Payload, syntax::Diagnostic> {
    if (parser.peek_token_is(syntax::TokenType::IDENT)) {
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        return TRY(IdentifierExpression::parse(parser));
    } else if (parser.peek_token_is(syntax::TokenType::STRING)) {
        TRY(parser.expect_peek(syntax::TokenType::STRING));
        const StringHandle string = TRY(StringExpression::parse(parser));

        if (string_is_empty(parser, string)) {
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
    auto imported_core = TRY(parse_import_core(parser));

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

    return StatementHandle{
        parser.get_ast().add_node(start_token, ImportStatement{imported_core, imported_alias})};
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
    return StatementHandle{parser.get_ast().add_node(start_token, ReturnStatement{value})};
}

auto TestStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    opt::Option<StringHandle> description;
    if (parser.peek_token_is(syntax::TokenType::STRING)) {
        parser.advance();
        description.emplace(TRY(StringExpression::parse(parser)));

        // Empty strings aren't supported since one should just use no description
        if (string_is_empty(parser, *description)) {
            return make_syntax_err(syntax::Error::EMPTY_TEST_DESCRIPTION,
                                   parser.get_location_of(**description));
        }
    }

    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    BlockHandle block = TRY(BlockStatement::parse(parser));
    return StatementHandle{
        parser.get_ast().add_node(start_token, TestStatement{description, block})};
}

auto UsingStatement::parse(syntax::Parser& parser) -> Result<StatementHandle, syntax::Diagnostic> {
    // A start token of public is guaranteed to be followed by an import
    const auto start_token = parser.get_current_token();
    if (parser.current_token_is(syntax::TokenType::PUBLIC)) { parser.advance(); }

    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    IdentifierHandle alias = TRY(IdentifierExpression::parse(parser));

    TRY(parser.expect_peek(syntax::TokenType::ASSIGN));
    auto type = TRY(ExplicitType::parse(parser));

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return StatementHandle{parser.get_ast().add_node(start_token, UsingStatement{alias, type})};
}

} // namespace porpoise::ast
