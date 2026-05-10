#include "ast/expression.hh"

#include "syntax/parser.hh"

namespace porpoise::ast {

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
    const auto item_type = TRY(ExplicitType::parse(parser));
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
    return parser.add_expr(start_token,
                           ArrayExpression{size, null_terminated, item_type, std::move(items)});
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
            arguments.emplace_back(*expr);
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

    return parser.add_expr(start_token, CallExpression{function, std::move(arguments)});
}

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
    const auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    if (parser.get_node<BlockStatement>(*block).empty()) {
        return make_syntax_err(syntax::Error::EMPTY_LOOP, parser.get_location_of(*block));
    }

    return parser.add_expr(start_token, DoWhileLoopExpression{block, condition});
}

namespace {

[[nodiscard]] static auto deconstruct_member(syntax::Parser& parser, StatementHandle member)
    -> Result<MemberHandle, syntax::Diagnostic> {
    switch (member->get_kind()) {
    case NodeKind::DECL_STATEMENT:
    case NodeKind::USING_STATEMENT:
    case NodeKind::IMPORT_STATEMENT: return MemberHandle{member};
    default:                         return make_syntax_err(syntax::Error::INVALID_MEMBER, parser.get_location_of(*member));
    }
}

template <typename MemberValidator>
[[nodiscard]] static auto parse_members(syntax::Parser& parser, MemberValidator&& validator)
    -> Result<Members, syntax::Diagnostic> {
    Members members;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        auto parsed_member = TRY(parser.parse_statement(true));

        // Downcast the parsed member into the specific member variant and check
        const auto member = TRY(deconstruct_member(parser, parsed_member));
        if (!std::forward<MemberValidator>(validator)(member)) {
            return make_syntax_err(syntax::Error::INVALID_MEMBER, parser.get_location_of(*member));
        }
        members.emplace_back(member);
    }
    return Members{members};
}

// Returns an actual value only if a terminal condition was found
[[nodiscard]] static auto validate_common_member_decl(const DeclStatement& decl) noexcept
    -> opt::Option<bool> {
    // Members that violate this wouldn't be usable with C
    if (decl.has_modifier(DeclModifiers::EXTERN) || decl.has_modifier(DeclModifiers::EXPORT)) {
        return false;
    }

    if (decl.value && (*decl.value)->is<FunctionExpression>()) { return true; }
    return opt::none;
}

[[nodiscard]] auto validate_struct_member_decl(const DeclStatement& decl) noexcept -> bool {
    // A non-static member must always be a variable to simplify the mental model
    if (const auto result = validate_common_member_decl(decl)) { return *result; }
    if (!decl.has_modifier(DeclModifiers::STATIC)) {
        return decl.has_modifier(DeclModifiers::VARIABLE);
    }
    return true;
}

[[nodiscard]] auto validate_non_struct_member_decl(const DeclStatement& decl) noexcept -> bool {
    if (const auto result = validate_common_member_decl(decl)) { return *result; }
    return decl.has_modifier(DeclModifiers::STATIC);
}

} // namespace

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
        enumerations.emplace_back(ident, value);

        // No comma means that its the end or that there is a decl list starting
        if (!parser.peek_token_is(syntax::TokenType::COMMA)) { break; }
        parser.advance();
    }

    auto members = TRY(parse_members(parser, [&](const MemberHandle& member) {
        return std::visit(Overloaded{[](const DeclStatement& decl) {
                                         return validate_non_struct_member_decl(decl);
                                     },
                                     [](const auto&) { return true; }},
                          parser.get_ast()[*member]);
    }));
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    // Validate here so that there aren't 3 errors spawning from an empty enum with decls
    if (enumerations.empty()) { return make_syntax_err(syntax::Error::EMPTY_ENUM, start_token); }
    return parser.add_expr(start_token,
                           EnumExpression{underlying, std::move(enumerations), std::move(members)});
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

        const auto iterable = TRY(parser.parse_expression());
        iterables.emplace_back(iterable);

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
                parser.add_node<DiscardableIdentHandle>(parser.get_current_token(), Unit{});
            captures.emplace_back(type_modifiers::VALUE, discarded);
        } else {
            // Always check for a modifier and advance past it if present
            const TypeModifier modifier{parser.get_current_token()};
            if (!modifier.is_value()) { parser.advance(); }

            const IdentifierHandle capture = TRY(IdentifierExpression::parse(parser));
            captures.emplace_back(modifier, capture);
        }

        if (!parser.peek_token_is(syntax::TokenType::BW_OR)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    TRY(parser.expect_peek(syntax::TokenType::BW_OR));

    // Loops must have a well formed block and may have an alternate in non-break cases
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    const BlockHandle block = TRY(BlockStatement::parse(parser));
    const auto        non_break =
        TRY(parser.try_parse_restricted_alternate(syntax::Error::ILLEGAL_LOOP_NON_BREAK));

    // The number of captures must align with the number of iterables
    if (captures.size() != iterables.size()) {
        return make_syntax_err(syntax::Error::FOR_ITERABLE_CAPTURE_MISMATCH, start_token);
    }

    if (parser.get_node<BlockStatement>(*block).empty()) {
        return make_syntax_err(syntax::Error::EMPTY_LOOP, parser.get_location_of(*block));
    }

    return parser.add_expr(
        start_token,
        ForLoopExpression{std::move(iterables), std::move(captures), block, non_break});
}

// Variadic must be handled first and should break the enclosing loop
auto FunctionExpression::try_parse_variadic(syntax::Parser& parser)
    -> Result<bool, syntax::Diagnostic> {
    bool is_variadic = false;
    if (parser.peek_token_is(syntax::TokenType::ELLIPSIS)) {
        parser.advance();
        is_variadic = true;
        if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    return is_variadic;
}

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
            const IdentifierHandle name         = TRY(IdentifierExpression::parse(parser));
            const auto [type_expr, initialized] = TRY(TypeExpression::parse(parser));
            const auto& type                    = parser.get_node<TypeExpression>(*type_expr);

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
    const auto return_type = TRY(ExplicitType::parse(parser));

    // If there is opening brace then just return without a body
    if (!parser.peek_token_is(syntax::TokenType::LBRACE)) {
        return make_syntax_err(syntax::Error::FN_DECLARATION_WITHOUT_BODY, start_token);
    }

    // Otherwise there must be a well-formed block
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    const BlockHandle body = TRY(BlockStatement::parse(parser));
    return parser.add_expr(
        start_token, FunctionExpression{self, std::move(parameters), variadic, return_type, body});
}

auto GroupedExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    parser.advance();
    const auto inner = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    return inner;
}

auto IdentifierExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    if (!start_token.is_valid_ident()) {
        return make_syntax_err(syntax::Error::ILLEGAL_IDENTIFIER, start_token);
    }

    return parser.add_expr(start_token, IdentifierExpression{start_token.slice});
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

    const auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    // The consequence and alternate are trivially handled by restricted statement parsers
    parser.advance();
    const auto consequence =
        TRY(parser.parse_restricted_statement(syntax::Error::ILLEGAL_IF_BRANCH, false));
    const auto alternate =
        TRY(parser.try_parse_restricted_alternate(syntax::Error::ILLEGAL_IF_BRANCH, false));

    return parser.add_expr(start_token,
                           IfExpression{constexpr_condition, condition, consequence, alternate});
}

auto IndexExpression::parse(syntax::Parser& parser, ExpressionHandle array)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::RBRACKET)) {
        return make_syntax_err(syntax::Error::INDEX_MISSING_EXPRESSION, start_token);
    }
    parser.advance();

    const auto idx_expr = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RBRACKET));
    return parser.add_expr(start_token, IndexExpression{array, idx_expr});
}

auto InfiniteLoopExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    const BlockHandle block = TRY(BlockStatement::parse(parser));
    if (parser.get_node<BlockStatement>(*block).empty()) {
        return make_syntax_err(syntax::Error::EMPTY_LOOP, parser.get_location_of(*block));
    }
    return parser.add_expr(start_token, InfiniteLoopExpression{block});
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
    const auto rhs = TRY(parser.parse_expression(current_precedence));
    return parser.add_expr(op_token, Node{lhs, rhs});
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
        const auto value = TRY(parser.parse_expression());
        initializers.emplace_back(member, value);

        if (!parser.peek_token_is(syntax::TokenType::RBRACE)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    return parser.add_expr(start_token, InitializerExpression{object, std::move(initializers)});
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

    return parser.add_expr(start_token, LabelExpression{name, body});
}

auto LabelExpression::deconstruct_body(syntax::Parser& parser, StatementHandle raw_stmt)
    -> Result<LabeledNodeHandle, syntax::Diagnostic> {
    switch (raw_stmt->get_kind()) {
    case NodeKind::EXPRESSION_STATEMENT: {
        const auto& expr_stmt = parser.get_node<ExpressionStatement>(*raw_stmt);
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

    const auto matcher = TRY(parser.parse_expression());
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

        const auto pattern = TRY(parser.parse_expression());
        TRY(parser.expect_peek(syntax::TokenType::FAT_ARROW));

        // There is an optional capture for every arm
        opt::Option<DiscardableIdentHandle> capture;
        if (parser.peek_token_is(syntax::TokenType::BW_OR)) {
            parser.advance();

            // An underscore is equivalent to a lack of capture
            if (parser.peek_token_is(syntax::TokenType::UNDERSCORE)) {
                parser.advance();
                capture.emplace(
                    parser.add_node<DiscardableIdentHandle>(parser.get_current_token(), Unit{}));
            } else {
                TRY(parser.expect_peek(syntax::TokenType::IDENT));
                capture.emplace(TRY(IdentifierExpression::parse(parser)));
            }
            TRY(parser.expect_peek(syntax::TokenType::BW_OR));
        }

        // The resulting statement must be restricted like an if branch
        parser.advance();
        const auto consequence =
            TRY(parser.parse_restricted_statement(syntax::Error::ILLEGAL_MATCH_ARM, false));
        arms.emplace_back(pattern, capture, consequence);
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    const auto catch_all =
        TRY(parser.try_parse_restricted_alternate(syntax::Error::ILLEGAL_MATCH_CATCH_ALL));

    return parser.add_expr(start_token, MatchExpression{matcher, std::move(arms), catch_all});
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

    const auto operand = TRY(parser.parse_expression(syntax::Precedence::PREFIX));
    return parser.add_expr(prefix_token, Node{operand});
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
    const auto operand = TRY(parser.parse_expression(syntax::Precedence::PREFIX));
    if (!operand->is<IdentifierExpression>()) {
        return make_syntax_err(syntax::Error::ILLEGAL_IMPLICIT_ACCESS_OPERAND,
                               parser.get_location_of(*operand));
    }

    return parser.add_expr(prefix_token, ImplicitAccessExpression{operand});
}

auto StringExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    return parser.add_expr(start_token, StringExpression{start_token.materialize_string()});
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
    return parser.add_expr(start_token, ScopeResolutionExpression{outer, inner});
}

auto StructExpression::parse(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    auto members = TRY(parse_members(parser, [&](const MemberHandle& member) {
        return std::visit(
            Overloaded{[](const DeclStatement& decl) { return validate_struct_member_decl(decl); },
                       [](const auto&) { return true; }},
            parser.get_ast()[*member]);
    }));

    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    return parser.add_expr(start_token, StructExpression{std::move(members)});
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
        const IdentifierHandle ident = TRY(IdentifierExpression::parse(parser));

        TRY(parser.expect_peek(syntax::TokenType::COLON));
        const auto type = TRY(ExplicitType::parse(parser));

        fields.emplace_back(ident, type);

        // No comma means that its the end or that there is a decl list starting
        if (!parser.peek_token_is(syntax::TokenType::COMMA)) { break; }
        parser.advance();
    }

    auto members = TRY(parse_members(parser, [&](const MemberHandle& member) {
        return std::visit(Overloaded{[](const DeclStatement& decl) {
                                         return validate_non_struct_member_decl(decl);
                                     },
                                     [](const auto&) { return true; }},
                          parser.get_ast()[*member]);
    }));
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    // Validate here so that there aren't 3 errors spawning from an empty union with decls
    if (fields.empty()) { return make_syntax_err(syntax::Error::EMPTY_UNION, start_token); }
    return parser.add_expr(start_token, UnionExpression{std::move(fields), std::move(members)});
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

    const auto condition = TRY(parser.parse_expression());
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
    const auto        non_break =
        TRY(parser.try_parse_restricted_alternate(syntax::Error::ILLEGAL_LOOP_NON_BREAK));

    // There needs to be at least a continuation or block
    if (!continuation && parser.get_node<BlockStatement>(*block).empty()) {
        return make_syntax_err(syntax::Error::EMPTY_LOOP, parser.get_location_of(*block));
    }

    return parser.add_expr(start_token,
                           WhileLoopExpression{condition, continuation, block, non_break});
}

} // namespace porpoise::ast
