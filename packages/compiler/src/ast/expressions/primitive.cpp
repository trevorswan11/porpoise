#include <cmath>
#include <limits>

#include "ast/expressions/primitive.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

// cppcheck-suppress-begin [constParameterReference, duplInheritedMember]

auto StringExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StringExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto promoted    = start_token.promote();
    if (!promoted) {
        return make_parser_unexpected(syntax::ParserError::MALFORMED_STRING, start_token);
    }

    return mem::make_box<StringExpression>(start_token, *promoted);
}

auto I32Expression::accept(Visitor& v) const -> void { v.visit(*this); }
auto I64Expression::accept(Visitor& v) const -> void { v.visit(*this); }
auto ISizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto U32Expression::accept(Visitor& v) const -> void { v.visit(*this); }
auto U64Expression::accept(Visitor& v) const -> void { v.visit(*this); }
auto USizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto U8Expression::accept(Visitor& v) const -> void { v.visit(*this); }

auto U8Expression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto slice       = start_token.slice;
    if (slice[1] != '\\') { return mem::make_box<U8Expression>(start_token, slice[1]); }

    const auto escaped = slice[2];
    byte       value;
    switch (escaped) {
    case 'n':  value = '\n'; break;
    case 'r':  value = '\r'; break;
    case 't':  value = '\t'; break;
    case '\\': value = '\\'; break;
    case '\'': value = '\''; break;
    case '"':  value = '"'; break;
    case '0':  value = '\0'; break;
    default:
        return make_parser_unexpected(syntax::ParserError::UNKNOWN_CHARACTER_ESCAPE, start_token);
    }

    return mem::make_box<U8Expression>(start_token, value);
}

template <typename T> auto approx_eq(T a, T b) -> bool {
    const auto largest = std::max(std::abs(b), std::abs(a));
    const auto diff    = std::abs(a - b);
    return diff <= largest * std::numeric_limits<T>::epsilon();
}

auto F32Expression::accept(Visitor& v) const -> void { v.visit(*this); }

auto F32Expression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<F32Expression>(other);
    return approx_eq(value_, casted.value_);
}

auto F64Expression::accept(Visitor& v) const -> void { v.visit(*this); }

auto F64Expression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<F64Expression>(other);
    return approx_eq(value_, casted.value_);
}

auto BoolExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto BoolExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto& start_token = parser.current_token();
    return mem::make_box<BoolExpression>(start_token, start_token.type == syntax::TokenType::TRUE);
}

// cppcheck-suppress-end [constParameterReference, duplInheritedMember]

} // namespace porpoise::ast
