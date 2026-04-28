#pragma once

#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "diagnostic/diagnostic.hpp"

#include "diagnostic/source_location.hpp"

#include "option.hpp"
#include "result.hpp"
#include "types.hpp"

namespace porpoise {

namespace syntax {

enum class TokenError : u8 {
    NON_STRING_TOKEN,
    UNEXPECTED_CHAR,
};

enum class TokenType : u8 {
    END,

    IDENT,

    INT_2,
    INT_8,
    INT_10,
    INT_16,
    LINT_2,
    LINT_8,
    LINT_10,
    LINT_16,
    ZINT_2,
    ZINT_8,
    ZINT_10,
    ZINT_16,

    UINT_2,
    UINT_8,
    UINT_10,
    UINT_16,
    ULINT_2,
    ULINT_8,
    ULINT_10,
    ULINT_16,
    UZINT_2,
    UZINT_8,
    UZINT_10,
    UZINT_16,

    F32,
    F64,
    STRING,
    U8,

    ASSIGN,
    WALRUS,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    BANG,

    BW_AND,
    BW_OR,
    SHL,
    SHR,
    NOT,
    XOR,

    PLUS_ASSIGN,
    MINUS_ASSIGN,
    STAR_ASSIGN,
    SLASH_ASSIGN,
    PERCENT_ASSIGN,
    BW_AND_ASSIGN,
    BW_OR_ASSIGN,
    SHL_ASSIGN,
    SHR_ASSIGN,
    NOT_ASSIGN,
    XOR_ASSIGN,

    LT,
    LT_EQ,
    GT,
    GT_EQ,
    EQ,
    NEQ,

    DOT,
    DOT_DOT,
    DOT_DOT_EQ,
    FAT_ARROW,

    COMMENT,
    MULTILINE_STRING,
    NULL_TERMINATED,

    COMMA,
    COLON,
    SEMICOLON,
    COLON_COLON,
    ELLIPSIS,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,

    SINGLE_QUOTE,
    UNDERSCORE,
    USING,
    AND_MUT,
    STAR_MUT,

    FUNCTION,
    VAR,
    CONSTANT,
    CONSTEXPR,
    STRUCT,
    ENUM,
    UNION,
    BOOLEAN_TRUE,
    BOOLEAN_FALSE,
    BOOLEAN_AND,
    BOOLEAN_OR,
    IF,
    ELSE,
    MATCH,
    RETURN,
    LOOP,
    FOR,
    WHILE,
    CONTINUE,
    BREAK,
    IMPORT,
    TYPE_TYPE,
    DO,
    AS,
    DEFER,
    TEST,

    I32_TYPE,
    I64_TYPE,
    ISIZE_TYPE,
    U32_TYPE,
    U64_TYPE,
    USIZE_TYPE,
    U8_TYPE,
    F32_TYPE,
    F64_TYPE,
    BOOL_TYPE,
    VOID_TYPE,

    PUBLIC,
    EXTERN,
    EXPORT,
    PACKED,
    VOLATILE,
    STATIC,
    NORETURN,
    NULLPTR,

    TYPEOF,
    SIZEOF,
    ALIGNOF,
    PTR_ADD,
    PTR_SUB,
    PTR_FROM_ARRAY,
    SLICE_FROM_PTR,
    PTR_IDX,
    PTR_FROM_INT,
    INT_FROM_PTR,
    SIN,
    COS,
    TAN,
    SQRT,
    LOG,
    LOG_10,
    LOG_2,
    MIN,
    MAX,
    MOD,
    DIVMOD,
    TRUNC,
    CAST,
    CEIL,
    FLOOR,
    EXP,
    EXP_2,
    POW,
    CLZ, // Count leading zeroes
    CTZ, // Count trailing zeroes

    ILLEGAL,
};

enum class Base : u8 {
    BINARY      = 2,
    OCTAL       = 8,
    DECIMAL     = 10,
    HEXADECIMAL = 16,
};

auto base_idx(Base base) noexcept -> int;
auto digit_in_base(byte c, Base base) noexcept -> bool;

namespace token_type {

enum class IntegerCategory : u8 {
    SIGNED_BASE,
    SIGNED_WIDE,
    SIGNED_SIZE,
    UNSIGNED_BASE,
    UNSIGNED_WIDE,
    UNSIGNED_SIZE,
};

consteval auto to_int_category(TokenType tt) noexcept -> IntegerCategory {
    switch (tt) {
    case TokenType::INT_2:
    case TokenType::INT_8:
    case TokenType::INT_10:
    case TokenType::INT_16:   return IntegerCategory::SIGNED_BASE;
    case TokenType::LINT_2:
    case TokenType::LINT_8:
    case TokenType::LINT_10:
    case TokenType::LINT_16:  return IntegerCategory::SIGNED_WIDE;
    case TokenType::ZINT_2:
    case TokenType::ZINT_8:
    case TokenType::ZINT_10:
    case TokenType::ZINT_16:  return IntegerCategory::SIGNED_SIZE;
    case TokenType::UINT_2:
    case TokenType::UINT_8:
    case TokenType::UINT_10:
    case TokenType::UINT_16:  return IntegerCategory::UNSIGNED_BASE;
    case TokenType::ULINT_2:
    case TokenType::ULINT_8:
    case TokenType::ULINT_10:
    case TokenType::ULINT_16: return IntegerCategory::UNSIGNED_WIDE;
    case TokenType::UZINT_2:
    case TokenType::UZINT_8:
    case TokenType::UZINT_10:
    case TokenType::UZINT_16: return IntegerCategory::UNSIGNED_SIZE;
    default:                  std::unreachable();
    }
}

auto to_base(TokenType tt) noexcept -> opt::Option<Base>;
auto misc_from_char(byte b) noexcept -> opt::Option<TokenType>;

constexpr auto is_i32(TokenType tt) noexcept -> bool {
    return TokenType::INT_2 <= tt && tt <= TokenType::INT_16;
}

constexpr auto is_i64(TokenType tt) noexcept -> bool {
    return TokenType::LINT_2 <= tt && tt <= TokenType::LINT_16;
}

constexpr auto is_isize_int(TokenType tt) noexcept -> bool {
    return TokenType::ZINT_2 <= tt && tt <= TokenType::ZINT_16;
}

constexpr auto is_u32(TokenType tt) noexcept -> bool {
    return TokenType::UINT_2 <= tt && tt <= TokenType::UINT_16;
}

constexpr auto is_u64(TokenType tt) noexcept -> bool {
    return TokenType::ULINT_2 <= tt && tt <= TokenType::ULINT_16;
}

constexpr auto is_usize_int(TokenType tt) noexcept -> bool {
    return TokenType::UZINT_2 <= tt && tt <= TokenType::UZINT_16;
}

constexpr auto is_int(TokenType tt) noexcept -> bool {
    return TokenType::INT_2 <= tt && tt <= TokenType::UZINT_16;
}

constexpr auto is_number(TokenType tt) noexcept -> bool {
    return is_int(tt) || tt == TokenType::F32 || tt == TokenType::F64;
}

auto suffix_length(TokenType tt) noexcept -> usize;

} // namespace token_type

using TokenDiagnostic = Diagnostic<TokenError>;

struct Token {
    TokenType        type{};
    std::string_view slice{};
    usize            line{};
    usize            column{};

    Token() noexcept = default;
    Token(TokenType tt, std::string_view tok) noexcept : type{tt}, slice{tok} {}
    Token(TokenType tt, std::string_view slice, usize line, usize column) noexcept
        : type{tt}, slice{slice}, line{line}, column{column} {}

    explicit Token(std::pair<TokenType, std::string_view> tok) noexcept
        : type{tok.first}, slice{tok.second} {}
    explicit Token(std::pair<std::string_view, TokenType> tok) noexcept
        : type{tok.second}, slice{tok.first} {}

    [[nodiscard]] auto promote() const -> Result<std::string, TokenDiagnostic>;
    [[nodiscard]] auto is_primitive() const noexcept -> bool;
    [[nodiscard]] auto is_builtin() const noexcept -> bool;
    [[nodiscard]] auto is_decl_token() const noexcept -> bool;

    // Check whether the token is an ident, primitive type, or builtin function.
    auto is_valid_ident() const noexcept -> bool;

    auto operator==(const Token& other) const noexcept -> bool {
        return type == other.type && slice == other.slice && line == other.line &&
               column == other.column;
    }
};

} // namespace syntax

template <> struct SourceInfo<syntax::Token> {
    static auto get(const syntax::Token& t) -> SourceLocation { return {t.line, t.column}; }
};

} // namespace porpoise

template <> struct fmt::formatter<porpoise::syntax::Token> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::syntax::Token& t, F& ctx) {
        return fmt::format_to(ctx.out(),
                              "{}({}) [{}, {}]",
                              magic_enum::enum_name(t.type),
                              t.slice,
                              t.line + 1,
                              t.column + 1);
    }
};
