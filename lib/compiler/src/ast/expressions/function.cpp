#include <algorithm>

#include "ast/expressions/function.hpp"

#include "ast/expressions/call.hpp" // IWYU pragma: keep
#include "ast/expressions/enum.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp" // IWYU pragma: keep
#include "ast/expressions/scope_resolve.hpp"
#include "ast/expressions/struct.hpp"
#include "ast/expressions/type.hpp"
#include "ast/expressions/union.hpp"
#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

SelfParameter::SelfParameter(TypeModifier modifier, mem::Box<IdentifierExpression> ident) noexcept
    : modifier_{std::move(modifier)}, ident_{std::move(ident)} {}
SelfParameter::~SelfParameter() = default;

auto SelfParameter::accept(Visitor& v) const -> void { v.visit(*this); }

auto SelfParameter::is_equal(const SelfParameter& other) const noexcept -> bool {
    return opt::safe_eq<TypeModifier>(modifier_, other.modifier_) && *ident_ == *other.ident_;
}

auto SelfParameter::get_token() const noexcept -> const syntax::Token& {
    return ident_->get_token();
}

FunctionParameter::FunctionParameter(mem::Box<IdentifierExpression> ident,
                                     ExplicitType&&                 type) noexcept
    : ident_{mem::nullable_box_from(std::move(ident))}, type_{std::move(type)} {}
FunctionParameter::FunctionParameter(ExplicitType&& type) noexcept : type_{std::move(type)} {}
FunctionParameter::~FunctionParameter() = default;

auto FunctionParameter::accept(Visitor& v) const -> void { v.visit(*this); }

auto FunctionParameter::is_equal(const FunctionParameter& other) const noexcept -> bool {
    return mem::nullable_boxes_eq(ident_, other.ident_) && type_ == other.type_;
}

auto FunctionParameter::get_token() const noexcept -> const syntax::Token& {
    if (ident_) { return ident_->get_token(); }
    return type_.get_token();
}

FunctionExpression::FunctionExpression(const syntax::Token&             start_token,
                                       opt::Option<SelfParameter>       self,
                                       std::vector<FunctionParameter>   parameters,
                                       bool                             variadic,
                                       ExplicitType&&                   return_type,
                                       mem::NullableBox<BlockStatement> body) noexcept
    : ExprBase{start_token}, self_{std::move(self)}, parameters_{std::move(parameters)},
      variadic_{variadic}, return_type_{std::move(return_type)}, body_{std::move(body)} {}
FunctionExpression::~FunctionExpression() = default;

auto FunctionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

// Variadic must be handled first and should break the enclosing loop
[[nodiscard]] static auto try_parse_variadic(syntax::Parser& parser)
    -> Result<bool, syntax::ParserDiagnostic> {
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

auto FunctionExpression::parse(syntax::Parser& parser)
    -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));

    // Parse the definition now that we're at the fn token
    opt::Option<SelfParameter>     self;
    std::vector<FunctionParameter> parameters;
    bool                           variadic = false;
    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        parser.advance();
    } else if ((variadic = TRY(try_parse_variadic(parser)))) {
        TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    } else {
        // The 'self' parameter can be a value type, ref, or mutable ref
        parser.advance();
        auto self_modifier = TypeModifier::from_token(parser.get_current_token());
        if (self_modifier.is_value() && (parser.peek_token_is(syntax::TokenType::COMMA) ||
                                         parser.peek_token_is(syntax::TokenType::RPAREN))) {
            self.emplace(SelfParameter{
                {}, downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)))});

            // Still end on a comma
            if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
                TRY(parser.expect_peek(syntax::TokenType::COMMA));
            }
        } else if (!self_modifier.is_value() && parser.peek_token_is(syntax::TokenType::IDENT)) {
            // Move up to the ident before parsing it
            parser.advance();
            self.emplace(SelfParameter{
                self_modifier,
                downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)))});

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
            if (!first || self.has_value()) { parser.advance(); }
            auto name = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
            auto [type_expr, initialized] = TRY(TypeExpression::parse(parser));
            auto type                     = downcast<TypeExpression>(std::move(type_expr));

            // There are no default values for parameters, and they must be explicitly typed
            if (initialized || !type->has_explicit_type()) {
                return make_parser_err(syntax::ParserError::FUNCTION_PARAMETER_HAS_DEFAULT_VALUE,
                                       type->get_token());
            }

            const auto& explicit_type = type->get_explicit_type();
            if (explicit_type.is_ident_type()) {
                // noreturn is not allowed for parameters
                if (explicit_type.get_ident_type().get_token().type ==
                    syntax::TokenType::NORETURN) {
                    return make_parser_err(syntax::ParserError::FUNCTION_PARAMETER_IS_NORETURN,
                                           type->get_token());
                }
            }

            parameters.emplace_back(std::move(name), std::move(*(type->explicit_)));
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
        return make_parser_err(syntax::ParserError::FN_DECLARATION_WITHOUT_BODY, start_token);
    }

    // Otherwise there must be a well-formed block
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    auto body = downcast<BlockStatement>(TRY(BlockStatement::parse(parser)));
    return mem::make_box<FunctionExpression>(start_token,
                                             std::move(self),
                                             std::move(parameters),
                                             variadic,
                                             std::move(return_type),
                                             mem::nullable_box_from(std::move(body)));
}

auto FunctionExpression::parse_type(syntax::Parser& parser)
    -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));

    // Parse the definition now that we're at the fn token
    std::vector<FunctionParameter> parameters;
    bool                           variadic = false;
    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        parser.advance();
    } else {
        // There is no self parameter for types
        while (!parser.peek_token_is(syntax::TokenType::RPAREN) &&
               !parser.peek_token_is(syntax::TokenType::END)) {
            if ((variadic = TRY(try_parse_variadic(parser)))) { break; }

            // There are no default values for parameters, and they must be explicitly typed
            auto type = TRY(ExplicitType::parse(parser));
            if (type.is_ident_type()) {
                // noreturn is not allowed for parameters
                const auto& token = type.get_ident_type().get_token();
                if (token.type == syntax::TokenType::NORETURN) {
                    return make_parser_err(syntax::ParserError::FUNCTION_PARAMETER_IS_NORETURN,
                                           token);
                }
            }

            parameters.emplace_back(std::move(type));
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
        return make_parser_err(syntax::ParserError::EXPLICIT_FN_TYPE_HAS_BODY, start_token);
    }

    return mem::make_box<FunctionExpression>(
        start_token, opt::none, std::move(parameters), variadic, std::move(return_type), nullptr);
}

auto FunctionExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted        = as<FunctionExpression>(other);
    const auto  self_matches  = opt::safe_eq<SelfParameter>(self_, casted.self_);
    const auto  parameters_eq = std::ranges::equal(parameters_, casted.parameters_);
    return self_matches && parameters_eq && variadic_ == casted.variadic_ &&
           return_type_ == casted.return_type_ && mem::nullable_boxes_eq(body_, casted.body_);
}

} // namespace porpoise::ast
