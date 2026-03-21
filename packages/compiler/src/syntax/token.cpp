#include <algorithm>
#include <cassert>
#include <cctype>

#include "syntax/keywords.hpp"
#include "syntax/token.hpp"

namespace porpoise::syntax {

auto base_idx(Base base) noexcept -> int {
    switch (base) {
    case Base::BINARY:      return 0;
    case Base::OCTAL:       return 1;
    case Base::DECIMAL:     return 2;
    case Base::HEXADECIMAL: return 3;
    }
}

auto digit_in_base(byte c, Base base) noexcept -> bool {
    switch (base) {
    case Base::BINARY:      return c == '0' || c == '1';
    case Base::OCTAL:       return c >= '0' && c <= '7';
    case Base::DECIMAL:     return std::isdigit(c);
    case Base::HEXADECIMAL: return std::isxdigit(c);
    default:                std::unreachable();
    }
}

namespace token_type {

auto to_base(syntax::TokenType tt) noexcept -> Optional<Base> {
    switch (tt) {
    case syntax::TokenType::INT_2:
    case syntax::TokenType::LINT_2:
    case syntax::TokenType::ZINT_2:
    case syntax::TokenType::UINT_2:
    case syntax::TokenType::ULINT_2:
    case syntax::TokenType::UZINT_2:  return Base::BINARY;
    case syntax::TokenType::INT_8:
    case syntax::TokenType::LINT_8:
    case syntax::TokenType::ZINT_8:
    case syntax::TokenType::UINT_8:
    case syntax::TokenType::ULINT_8:
    case syntax::TokenType::UZINT_8:  return Base::OCTAL;
    case syntax::TokenType::INT_10:
    case syntax::TokenType::LINT_10:
    case syntax::TokenType::ZINT_10:
    case syntax::TokenType::UINT_10:
    case syntax::TokenType::ULINT_10:
    case syntax::TokenType::UZINT_10: return Base::DECIMAL;
    case syntax::TokenType::INT_16:
    case syntax::TokenType::LINT_16:
    case syntax::TokenType::ZINT_16:
    case syntax::TokenType::UINT_16:
    case syntax::TokenType::ULINT_16:
    case syntax::TokenType::UZINT_16: return Base::HEXADECIMAL;
    default:                          return std::nullopt;
    }
}

auto misc_from_char(byte c) noexcept -> Optional<syntax::TokenType> {
    switch (c) {
    case ',': return syntax::TokenType::COMMA;
    case ':': return syntax::TokenType::COLON;
    case ';': return syntax::TokenType::SEMICOLON;
    case '(': return syntax::TokenType::LPAREN;
    case ')': return syntax::TokenType::RPAREN;
    case '{': return syntax::TokenType::LBRACE;
    case '}': return syntax::TokenType::RBRACE;
    case '[': return syntax::TokenType::LBRACKET;
    case ']': return syntax::TokenType::RBRACKET;
    case '_': return syntax::TokenType::UNDERSCORE;
    default:  return std::nullopt;
    }
}

using SuffixMapping                = std::pair<bool (*)(syntax::TokenType), usize>;
constexpr auto INT_SUFFIX_MAPPINGS = std::to_array<SuffixMapping>({
    {is_signed_int, 0},
    {is_signed_long_int, 1},
    {is_isize_int, 1},
    {is_unsigned_int, 1},
    {is_unsigned_long_int, 2},
    {is_usize_int, 2},
});

auto suffix_length(syntax::TokenType tt) noexcept -> usize {
    if (tt == syntax::TokenType::FLOAT) { return 1; }
    if (tt < syntax::TokenType::INT_2 || tt > syntax::TokenType::UZINT_16) { return 0; }
    return std::ranges::find_if(
               INT_SUFFIX_MAPPINGS,
               [tt](auto in_range) { return in_range(tt); },
               &SuffixMapping::first)
        ->second;
}

} // namespace token_type

auto syntax::Token::promote() const -> Expected<std::string, Diagnostic<TokenError>> {
    if (type != syntax::TokenType::STRING && type != syntax::TokenType::MULTILINE_STRING) {
        return Unexpected{Diagnostic{TokenError::NON_STRING_TOKEN, line, column}};
    }

    // Here we can just trim off the start and finish of the string
    if (type == syntax::TokenType::STRING) {
        if (slice.size() < 2) {
            return Unexpected{Diagnostic{TokenError::UNEXPECTED_CHAR, line, column}};
        }
        return std::string{slice.begin() + 1, slice.end() - 1};
    }

    std::string builder{};
    builder.reserve(slice.size());

    auto at_line_start = true;
    for (size_t i = 0; i < slice.size(); ++i) {
        const auto c = slice[i];

        // Skip a double backslash at start of line to clean the string
        if (at_line_start) {
            if (c == '\\' && i + 1 < slice.size() && slice[i + 1] == '\\') {
                i += 1;
                continue;
            }
            at_line_start = false;
        }

        builder.push_back(c);
        if (c == '\n') { at_line_start = true; }
    }

    return builder;
}

auto syntax::Token::is_primitive() const noexcept -> bool {
    return std::ranges::contains(ALL_PRIMITIVES, type);
}

auto syntax::Token::is_builtin() const noexcept -> bool {
    return std::ranges::contains(ALL_BUILTINS, type, &syntax::Keyword::second);
}

auto syntax::Token::is_valid_ident() const noexcept -> bool {
    return type == syntax::TokenType::IDENT || type == syntax::TokenType::NORETURN ||
           type == syntax::TokenType::TYPE_TYPE || is_primitive() || is_builtin();
}

} // namespace porpoise::syntax
