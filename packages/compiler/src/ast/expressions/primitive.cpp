#include <cmath>
#include <limits>

#include "ast/expressions/primitive.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

// cppcheck-suppress-begin [constParameterReference, duplInheritedMember]

auto StringExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StringExpression::parse(syntax::Parser& parser)
    -> Expected<Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto promoted    = start_token.promote();
    if (!promoted) {
        return make_parser_unexpected(syntax::ParserError::MALFORMED_STRING, start_token);
    }

    return make_box<StringExpression>(start_token, *promoted);
}

auto SignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto SignedLongIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto ISizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto UnsignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto UnsignedLongIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto USizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ByteExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ByteExpression::parse(syntax::Parser& parser)
    -> Expected<Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto slice       = start_token.slice;
    if (slice[1] != '\\') { return make_box<ByteExpression>(start_token, slice[1]); }

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

    return make_box<ByteExpression>(start_token, value);
}

template <typename T> auto approx_eq(T a, T b) -> bool {
    const auto largest = std::max(std::abs(b), std::abs(a));
    const auto diff    = std::abs(a - b);
    return diff <= largest * std::numeric_limits<T>::epsilon();
}

auto FloatExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto FloatExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<FloatExpression>(other);
    return approx_eq(value_, casted.value_);
}

auto DoubleExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto DoubleExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<DoubleExpression>(other);
    return approx_eq(value_, casted.value_);
}

auto BoolExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto BoolExpression::parse(syntax::Parser& parser)
    -> Expected<Box<Expression>, syntax::ParserDiagnostic> {
    const auto& start_token = parser.current_token();
    return make_box<BoolExpression>(start_token, start_token.type == syntax::TokenType::TRUE);
}

// cppcheck-suppress-end [constParameterReference, duplInheritedMember]

} // namespace porpoise::ast
