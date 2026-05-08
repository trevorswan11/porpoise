#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include "syntax/token.hh"

#include "option.hh"

namespace porpoise::syntax {

using Keyword = std::pair<std::string_view, TokenType>;

namespace keywords {

constexpr Keyword FN{"fn", TokenType::FUNCTION};
constexpr Keyword VAR{"var", TokenType::VAR};
constexpr Keyword CONSTANT{"const", TokenType::CONSTANT};
constexpr Keyword CONSTEXPR{"constexpr", TokenType::CONSTEXPR};
constexpr Keyword STRUCT{"struct", TokenType::STRUCT};
constexpr Keyword ENUM{"enum", TokenType::ENUM};
constexpr Keyword UNION{"union", TokenType::UNION};
constexpr Keyword BOOLEAN_TRUE{"true", TokenType::BOOLEAN_TRUE};
constexpr Keyword BOOLEAN_FALSE{"false", TokenType::BOOLEAN_FALSE};
constexpr Keyword IF{"if", TokenType::IF};
constexpr Keyword ELSE{"else", TokenType::ELSE};
constexpr Keyword DO{"do", TokenType::DO};
constexpr Keyword MATCH{"match", TokenType::MATCH};
constexpr Keyword RETURN{"return", TokenType::RETURN};
constexpr Keyword DEFER{"defer", TokenType::DEFER};
constexpr Keyword LOOP{"loop", TokenType::LOOP};
constexpr Keyword FOR{"for", TokenType::FOR};
constexpr Keyword WHILE{"while", TokenType::WHILE};
constexpr Keyword CONTINUE{"continue", TokenType::CONTINUE};
constexpr Keyword BREAK{"break", TokenType::BREAK};
constexpr Keyword IMPORT{"import", TokenType::IMPORT};
constexpr Keyword I32{"i32", TokenType::I32_TYPE};
constexpr Keyword I64{"i64", TokenType::I64_TYPE};
constexpr Keyword ISIZE{"isize", TokenType::ISIZE_TYPE};
constexpr Keyword U32{"u32", TokenType::U32_TYPE};
constexpr Keyword U64{"u64", TokenType::U64_TYPE};
constexpr Keyword USIZE{"usize", TokenType::USIZE_TYPE};
constexpr Keyword F32{"f32", TokenType::F32_TYPE};
constexpr Keyword F64{"f64", TokenType::F64_TYPE};
constexpr Keyword U8{"u8", TokenType::U8_TYPE};
constexpr Keyword BOOL{"bool", TokenType::BOOL_TYPE};
constexpr Keyword VOID{"void", TokenType::VOID_TYPE};
constexpr Keyword TYPE{"type", TokenType::TYPE_TYPE};
constexpr Keyword AUTO{"auto", TokenType::AUTO_TYPE};
constexpr Keyword OPAQUE{"opaque", TokenType::OPAQUE_TYPE};
constexpr Keyword AS{"as", TokenType::AS};
constexpr Keyword PUBLIC{"pub", TokenType::PUBLIC};
constexpr Keyword EXTERN{"extern", TokenType::EXTERN};
constexpr Keyword EXPORT{"export", TokenType::EXPORT};
constexpr Keyword VOLATILE{"volatile", TokenType::VOLATILE};
constexpr Keyword STATIC{"static", TokenType::STATIC};
constexpr Keyword NORETURN{"noreturn", TokenType::NORETURN};
constexpr Keyword NULLPTR{"nullptr", TokenType::NULLPTR};
constexpr Keyword USING{"using", TokenType::USING};
constexpr Keyword TEST{"test", TokenType::TEST};

} // namespace keywords

constexpr auto ALL_KEYWORDS = [] {
    auto all_keywords = std::array{
        keywords::FN,        keywords::VAR,          keywords::CONSTANT,
        keywords::CONSTEXPR, keywords::STRUCT,       keywords::ENUM,
        keywords::UNION,     keywords::BOOLEAN_TRUE, keywords::BOOLEAN_FALSE,
        keywords::IF,        keywords::ELSE,         keywords::DO,
        keywords::MATCH,     keywords::RETURN,       keywords::DEFER,
        keywords::LOOP,      keywords::FOR,          keywords::WHILE,
        keywords::CONTINUE,  keywords::BREAK,        keywords::IMPORT,
        keywords::I32,       keywords::I64,          keywords::ISIZE,
        keywords::U32,       keywords::U64,          keywords::USIZE,
        keywords::F32,       keywords::F64,          keywords::U8,
        keywords::BOOL,      keywords::VOID,         keywords::TYPE,
        keywords::AUTO,      keywords::OPAQUE,       keywords::AS,
        keywords::PUBLIC,    keywords::EXTERN,       keywords::EXPORT,
        keywords::VOLATILE,  keywords::STATIC,       keywords::NORETURN,
        keywords::NULLPTR,   keywords::USING,        keywords::TEST,
    };

    std::ranges::sort(all_keywords, {}, &Keyword::first);
    return all_keywords;
}();

constexpr auto get_keyword(std::string_view sv) noexcept -> opt::Option<Keyword> {
    const auto it = std::ranges::lower_bound(ALL_KEYWORDS, sv, {}, &Keyword::first);
    if (it == ALL_KEYWORDS.end() || it->first != sv) { return opt::none; }
    return opt::Option<Keyword>{*it};
}

constexpr auto ALL_PRIMITIVES = std::array{
    keywords::I32.second,
    keywords::I64.second,
    keywords::ISIZE.second,
    keywords::U32.second,
    keywords::U64.second,
    keywords::USIZE.second,
    keywords::F32.second,
    keywords::F64.second,
    keywords::U8.second,
    keywords::BOOL.second,
    keywords::VOID.second,
};

} // namespace porpoise::syntax
