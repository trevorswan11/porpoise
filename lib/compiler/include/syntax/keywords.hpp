#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include "syntax/token.hpp"

#include "option.hpp"

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
constexpr Keyword PACKED{"packed", TokenType::PACKED};
constexpr Keyword VOLATILE{"volatile", TokenType::VOLATILE};
constexpr Keyword STATIC{"static", TokenType::STATIC};
constexpr Keyword NORETURN{"noreturn", TokenType::NORETURN};
constexpr Keyword NULLPTR{"nullptr", TokenType::NULLPTR};
constexpr Keyword USING{"using", TokenType::USING};
constexpr Keyword TEST{"test", TokenType::TEST};

namespace builtins {

constexpr Keyword ALIGN_CAST{"@alignCast", TokenType::BUILTIN_ALIGN_CAST};
constexpr Keyword PTR_CAST{"@ptrCast", TokenType::BUILTIN_PTR_CAST};
constexpr Keyword BIT_CAST{"@bitCast", TokenType::BUILTIN_BIT_CAST};
constexpr Keyword CONST_CAST{"@constCast", TokenType::BUILTIN_CONST_CAST};
constexpr Keyword VOLATILE_CAST{"@volatileCast", TokenType::BUILTIN_VOLATILE_CAST};
constexpr Keyword AS{"@as", TokenType::BUILTIN_AS};
constexpr Keyword INT_FROM_PTR{"@intFromPtr", TokenType::BUILTIN_INT_FROM_PTR};
constexpr Keyword PTR_FROM_INT{"@ptrFromInt", TokenType::BUILTIN_PTR_FROM_INT};
constexpr Keyword PTR_FROM_ARRAY{"@ptrFromArray", TokenType::BUILTIN_PTR_FROM_ARRAY};
constexpr Keyword SLICE_FROM_PTR{"@sliceFromPtr", TokenType::BUILTIN_SLICE_FROM_PTR};

constexpr Keyword ALIGN_OF{"@alignOf", TokenType::BUILTIN_ALIGN_OF};
constexpr Keyword SIZE_OF{"@sizeOf", TokenType::BUILTIN_SIZE_OF};
constexpr Keyword TYPE_OF{"@typeOf", TokenType::BUILTIN_TYPE_OF};
constexpr Keyword TAG_NAME{"@tagName", TokenType::BUILTIN_TAG_NAME};

constexpr Keyword MEMCPY{"@memcpy", TokenType::BUILTIN_MEMCPY};
constexpr Keyword MEMSET{"@memset", TokenType::BUILTIN_MEMSET};
constexpr Keyword MEMMOVE{"@memmove", TokenType::BUILTIN_MEMMOVE};

constexpr Keyword MUL_ADD{"@mulAdd", TokenType::BUILTIN_MUL_ADD};
constexpr Keyword CLZ{"@clz", TokenType::BUILTIN_CLZ};
constexpr Keyword CTZ{"@ctz", TokenType::BUILTIN_CTZ};
constexpr Keyword DIV_MOD{"@divMod", TokenType::BUILTIN_DIV_MOD};
constexpr Keyword POP_COUNT{"@popCount", TokenType::BUILTIN_POP_COUNT};
constexpr Keyword SQRT{"@sqrt", TokenType::BUILTIN_SQRT};
constexpr Keyword SIN{"@sin", TokenType::BUILTIN_SIN};
constexpr Keyword COS{"@cos", TokenType::BUILTIN_COS};
constexpr Keyword TAN{"@tan", TokenType::BUILTIN_TAN};
constexpr Keyword EXP{"@exp", TokenType::BUILTIN_EXP};
constexpr Keyword EXP2{"@exp2", TokenType::BUILTIN_EXP2};
constexpr Keyword LOG{"@log", TokenType::BUILTIN_LOG};
constexpr Keyword LOG2{"@log2", TokenType::BUILTIN_LOG2};
constexpr Keyword LOG10{"@log10", TokenType::BUILTIN_LOG10};
constexpr Keyword ABS{"@abs", TokenType::BUILTIN_ABS};
constexpr Keyword FLOOR{"@floor", TokenType::BUILTIN_FLOOR};
constexpr Keyword CEIL{"@ceil", TokenType::BUILTIN_CEIL};

constexpr Keyword PANIC{"@panic", TokenType::BUILTIN_PANIC};

} // namespace builtins

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
        keywords::PACKED,    keywords::VOLATILE,     keywords::STATIC,
        keywords::NORETURN,  keywords::NULLPTR,      keywords::USING,
        keywords::TEST,
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

constexpr auto ALL_BUILTINS = [] {
    namespace builtins = keywords::builtins;
    auto all_builtins  = std::array{
        builtins::ALIGN_CAST,
        builtins::PTR_CAST,
        builtins::BIT_CAST,
        builtins::CONST_CAST,
        builtins::VOLATILE_CAST,
        builtins::AS,
        builtins::INT_FROM_PTR,
        builtins::PTR_FROM_INT,
        builtins::PTR_FROM_ARRAY,
        builtins::SLICE_FROM_PTR,
        builtins::ALIGN_OF,
        builtins::SIZE_OF,
        builtins::TYPE_OF,
        builtins::TAG_NAME,
        builtins::MEMCPY,
        builtins::MEMSET,
        builtins::MEMMOVE,
        builtins::MUL_ADD,
        builtins::CLZ,
        builtins::CTZ,
        builtins::DIV_MOD,
        builtins::PANIC,
        builtins::POP_COUNT,
        builtins::SQRT,
        builtins::SIN,
        builtins::COS,
        builtins::TAN,
        builtins::EXP,
        builtins::EXP2,
        builtins::LOG,
        builtins::LOG2,
        builtins::LOG10,
        builtins::ABS,
        builtins::FLOOR,
        builtins::CEIL,
    };

    std::ranges::sort(all_builtins, {}, &Keyword::first);
    return all_builtins;
}();

constexpr auto get_builtin(std::string_view sv) noexcept -> opt::Option<Keyword> {
    const auto it = std::ranges::lower_bound(ALL_BUILTINS, sv, {}, &Keyword::first);
    if (it == ALL_BUILTINS.end() || it->first != sv) { return opt::none; }
    return opt::Option<Keyword>{*it};
}

} // namespace porpoise::syntax
