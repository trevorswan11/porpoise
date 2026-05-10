#include <charconv>

#include "ast/primitive.hh"

#include "syntax/parser.hh"

namespace porpoise::ast {

namespace {

// Parses the requested value from the string, asserting the from_chars result if requested
template <typename ValueType, bool AssertLast = true>
[[nodiscard]] static auto parse_primitive_value(std::string_view  slice,
                                                syntax::TokenType type) noexcept
    -> opt::Option<ValueType> {
    const auto  base  = syntax::token_type::to_base(type);
    const auto* first = slice.cbegin() + (!base || *base == syntax::Base::DECIMAL ? 0 : 2);
    const auto* last  = slice.cend() - syntax::token_type::suffix_length(type);

    ValueType              v;
    std::from_chars_result result;
    if constexpr (std::is_same_v<ValueType, f64> || std::is_same_v<ValueType, f32>) {
        result = std::from_chars(first, last, v);
    } else {
        result = std::from_chars(first, last, v, std::to_underlying(*base));
    }

    if constexpr (AssertLast) {
        ASSERT(result.ptr == last);
        if (result.ec == std::errc{}) { return v; }
    } else {
        if (result.ec == std::errc{} && result.ptr == last) { return v; }
    }

    ASSERT(result.ec == std::errc::result_out_of_range);
    return opt::none;
}

template <traits::ValuedPrimitiveNode Node>
static auto parse_primitive(syntax::Parser& parser)
    -> Result<ExpressionHandle, syntax::Diagnostic> {
    using value_type       = typename Node::value_type;
    const auto start_token = parser.get_current_token();
    const auto value       = parse_primitive_value<value_type>(start_token.slice, start_token.type);
    if (value) { return parser.add_expr(start_token, Node{*value}); }

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
        return parser.add_expr(start_token, U8Expression{static_cast<u8>(slice[1])});
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

    return parser.add_expr(start_token, U8Expression{value});
}

MAKE_PRIMITIVE_PARSER(F32Expression)
MAKE_PRIMITIVE_PARSER(F64Expression)

auto BoolExpression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    return parser.add_expr(parser.get_current_token(), BoolExpression{});
}

auto VoidExpression::parse(syntax::Parser& parser) -> Result<ExpressionHandle, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    return parser.add_expr(start_token, VoidExpression{});
}

} // namespace porpoise::ast
