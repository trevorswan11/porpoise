#include "ast/expressions/type.hpp"

#include "ast/expressions/enum.hpp"
#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/struct.hpp"
#include "ast/expressions/union.hpp"
#include "ast/visitor.hpp"
#include <type_traits>

namespace porpoise::ast {

ExplicitArrayType::ExplicitArrayType(Optional<mem::Box<Expression>> dimension,
                                     bool                           null_terminated,
                                     mem::Box<ExplicitType>         inner_type) noexcept
    : dimension_{std::move(dimension)}, null_terminated_{null_terminated},
      inner_type_{std::move(inner_type)} {}
ExplicitArrayType::~ExplicitArrayType() = default;

auto ExplicitArrayType::get_token() const noexcept -> const syntax::Token& {
    return inner_type_->get_token();
}

auto ExplicitArrayType::is_equal(const ExplicitArrayType& other) const noexcept -> bool {
    return optional::unsafe_eq<Expression>(dimension_, other.dimension_) &&
           *inner_type_ == *other.inner_type_;
}

ExplicitType::ExplicitType(TypeModifier modifier, ExplicitTypeVariant type) noexcept
    : modifier_{std::move(modifier)}, type_{std::move(type)} {}
ExplicitType::~ExplicitType() = default;

auto ExplicitType::accept(Visitor& v) const -> void { v.visit(*this); }

[[nodiscard]] auto ExplicitType::parse(syntax::Parser& parser)
    -> Expected<ExplicitType, syntax::ParserDiagnostic> {
    // Always check for a modifier and advance past it if present
    const auto modifier_token = parser.peek_token();
    const auto modifier       = TypeModifier::from_token(modifier_token);
    if (!modifier.is_value()) { parser.advance(); }

    // The array dimension of a type are only present conditionally
    if (parser.peek_token_is(syntax::TokenType::LBRACKET)) {
        parser.advance();

        auto                           null_terminated = false;
        Optional<mem::Box<Expression>> dimension;
        if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
            parser.advance();
            null_terminated = true;
        } else if (!parser.peek_token_is(syntax::TokenType::RBRACKET)) {
            parser.advance();
            dimension.emplace(TRY(parser.parse_expression()));
            if (!(*dimension)->any<USizeIntegerExpression, IdentifierExpression>()) {
                return make_parser_unexpected(syntax::ParserError::ILLEGAL_ARRAY_SIZE_TYPE,
                                              (*dimension)->get_token());
            }

            // The null terminated marker comes after the size for explicitly sized types
            if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
                parser.advance();
                null_terminated = true;
            }
        }
        TRY(parser.expect_peek(syntax::TokenType::RBRACKET));

        // Arrays are recursively defined
        auto inner = TRY(ExplicitType::parse(parser));
        return ExplicitType{std::move(modifier),
                            ExplicitArrayType{std::move(dimension),
                                              null_terminated,
                                              mem::make_box<ExplicitType>(std::move(inner))}};
    } else if (!TypeModifier::from_token(parser.peek_token()).is_value()) {
        // Don't advance since the parser does it implicitly here (costs two from_token calls)
        auto inner = TRY(ExplicitType::parse(parser));
        return ExplicitType{std::move(modifier), mem::make_box<ExplicitType>(std::move(inner))};
    }

    // Otherwise the type has to be a 'simple' function or ident
    const auto& peek_token = parser.peek_token();
    if (peek_token.is_valid_ident() && !peek_token.is_builtin()) {
        // It's trivial to catch these syntactic errors here
        if (!modifier.is_value()) {
            switch (peek_token.type) {
            case syntax::TokenType::TYPE_TYPE:
                return make_parser_unexpected(syntax::ParserError::ILLEGAL_TYPE_TYPE_MODIFIER,
                                              modifier_token);
            case syntax::TokenType::VOID_TYPE:
                return make_parser_unexpected(syntax::ParserError::ILLEGAL_VOID_TYPE_MODIFIER,
                                              modifier_token);
            case syntax::TokenType::NORETURN:
                return make_parser_unexpected(syntax::ParserError::ILLEGAL_NORETURN_TYPE_MODIFIER,
                                              modifier_token);
            default: break;
            }
        }

        parser.advance();
        return ExplicitType{
            modifier,
            Node::downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)))};
    }

    // The inner type is limited to functions and user-defined types
    const auto type_start = parser.current_token();
    if (parser.peek_token_is(syntax::TokenType::FUNCTION)) {
        parser.advance();
        auto type_expr = TRY(FunctionExpression::parse_type(parser));

        auto function = Node::downcast<FunctionExpression>(std::move(type_expr));
        if (!(modifier.is_value() || modifier.is_ptr())) {
            return make_parser_unexpected(syntax::ParserError::ILLEGAL_FUNCTION_TYPE_MODIFIER,
                                          type_start);
        }

        // Function types cannot have bodies
        assert(!function->has_body() && "Function type has body");
        return ExplicitType{modifier, std::move(function)};
    }

    // The user-defined types can be handled by parsing any expression and verifying it
    switch (auto user = TRY(parser.parse_expression()); user->get_kind()) {
    case NodeKind::STRUCT_EXPRESSION:
        return ExplicitType{modifier, Node::downcast<StructExpression>(std::move(user))};
    case NodeKind::ENUM_EXPRESSION:
        return ExplicitType{modifier, Node::downcast<EnumExpression>(std::move(user))};
    case NodeKind::UNION_EXPRESSION:
        return ExplicitType{modifier, Node::downcast<UnionExpression>(std::move(user))};
    default: break;
    }
    return make_parser_unexpected(syntax::ParserError::ILLEGAL_EXPLICIT_TYPE, type_start);
}

auto ExplicitType::get_token() const noexcept -> const syntax::Token& {
    return match(Overloaded{
        [](const auto& t) -> const syntax::Token& { return t->get_token(); },
        [](const ExplicitArrayType& a) -> const syntax::Token& { return a.get_token(); }});
}

auto ExplicitType::is_equal(const ExplicitType& other) const noexcept -> bool {
    const auto& other_type = other.type_;
    if (type_.index() != other_type.index()) { return false; }
    return modifier_ == other.modifier_ &&
           std::visit(Overloaded{
                          [&other_type](const auto& v) {
                              return *v == *std::get<std::remove_cvref_t<decltype(v)>>(other_type);
                          },
                          [&other_type](const ExplicitArrayType& v) {
                              return v == std::get<ExplicitArrayType>(other_type);
                          },
                      },
                      type_);
}

TypeExpression::TypeExpression(const syntax::Token&   start_token,
                               Optional<ExplicitType> exp) noexcept
    : ExprBase{start_token}, explicit_{std::move(exp)} {}
TypeExpression::~TypeExpression() = default;

auto TypeExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto TypeExpression::parse(syntax::Parser& parser)
    -> Expected<std::pair<mem::Box<Expression>, bool>, syntax::ParserDiagnostic> {
    // The start start token is always offset as this is called irregularly
    const auto start_token = parser.peek_token();

    auto [type, initialized] = TRY(
        ([&]() -> Expected<std::pair<mem::Box<TypeExpression>, bool>, syntax::ParserDiagnostic> {
            if (parser.peek_token_is(syntax::TokenType::WALRUS)) {
                auto type_expr = mem::make_box<TypeExpression>(start_token, std::nullopt);
                parser.advance();
                return std::pair{std::move(type_expr), true};
            } else if (parser.peek_token_is(syntax::TokenType::COLON)) {
                parser.advance();
                auto explicit_type = TRY(ExplicitType::parse(parser));
                auto type_expr =
                    mem::make_box<TypeExpression>(start_token, std::move(explicit_type));
                if (parser.peek_token_is(syntax::TokenType::ASSIGN)) {
                    parser.advance();
                    return std::pair{std::move(type_expr), true};
                }
                return std::pair{std::move(type_expr), false};
            } else {
                return Unexpected{parser.peek_error(syntax::TokenType::COLON)};
            }
        }()));

    // Advance again to prepare for rhs
    if (initialized) { parser.advance(); }
    return std::pair{mem::box_into<Expression>(std::move(type)), initialized};
}

auto TypeExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<TypeExpression>(other);
    return optional::safe_eq<ExplicitType>(explicit_, casted.explicit_);
}

} // namespace porpoise::ast
