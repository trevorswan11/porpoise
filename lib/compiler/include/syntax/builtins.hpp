#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include "syntax/token.hpp"

#include "option.hpp"

namespace porpoise::syntax {

using Builtin = std::pair<std::string_view, TokenType>;

namespace builtins {

constexpr Builtin ALIGN_CAST{"@alignCast", TokenType::BUILTIN_ALIGN_CAST};
constexpr Builtin PTR_CAST{"@ptrCast", TokenType::BUILTIN_PTR_CAST};
constexpr Builtin BIT_CAST{"@bitCast", TokenType::BUILTIN_BIT_CAST};
constexpr Builtin CONST_CAST{"@constCast", TokenType::BUILTIN_CONST_CAST};
constexpr Builtin VOLATILE_CAST{"@volatileCast", TokenType::BUILTIN_VOLATILE_CAST};
constexpr Builtin AS{"@as", TokenType::BUILTIN_AS};

constexpr Builtin INT_FROM_PTR{"@intFromPtr", TokenType::BUILTIN_INT_FROM_PTR};
constexpr Builtin PTR_FROM_INT{"@ptrFromInt", TokenType::BUILTIN_PTR_FROM_INT};
constexpr Builtin PTR_FROM_ARRAY{"@ptrFromArray", TokenType::BUILTIN_PTR_FROM_ARRAY};
constexpr Builtin SLICE_FROM_PTR{"@sliceFromPtr", TokenType::BUILTIN_SLICE_FROM_PTR};

constexpr Builtin ALIGN_OF{"@alignOf", TokenType::BUILTIN_ALIGN_OF};
constexpr Builtin SIZE_OF{"@sizeOf", TokenType::BUILTIN_SIZE_OF};
constexpr Builtin TYPE_OF{"@typeOf", TokenType::BUILTIN_TYPE_OF};
constexpr Builtin TAG_NAME{"@tagName", TokenType::BUILTIN_TAG_NAME};

constexpr Builtin MEMCPY{"@memcpy", TokenType::BUILTIN_MEMCPY};
constexpr Builtin MEMSET{"@memset", TokenType::BUILTIN_MEMSET};
constexpr Builtin MEMMOVE{"@memmove", TokenType::BUILTIN_MEMMOVE};

constexpr Builtin MUL_ADD{"@mulAdd", TokenType::BUILTIN_MUL_ADD};
constexpr Builtin CLZ{"@clz", TokenType::BUILTIN_CLZ};
constexpr Builtin CTZ{"@ctz", TokenType::BUILTIN_CTZ};
constexpr Builtin DIV_MOD{"@divMod", TokenType::BUILTIN_DIV_MOD};
constexpr Builtin POP_COUNT{"@popCount", TokenType::BUILTIN_POP_COUNT};
constexpr Builtin SQRT{"@sqrt", TokenType::BUILTIN_SQRT};
constexpr Builtin SIN{"@sin", TokenType::BUILTIN_SIN};
constexpr Builtin COS{"@cos", TokenType::BUILTIN_COS};
constexpr Builtin TAN{"@tan", TokenType::BUILTIN_TAN};
constexpr Builtin EXP{"@exp", TokenType::BUILTIN_EXP};
constexpr Builtin EXP2{"@exp2", TokenType::BUILTIN_EXP2};
constexpr Builtin LOG{"@log", TokenType::BUILTIN_LOG};
constexpr Builtin LOG2{"@log2", TokenType::BUILTIN_LOG2};
constexpr Builtin LOG10{"@log10", TokenType::BUILTIN_LOG10};
constexpr Builtin ABS{"@abs", TokenType::BUILTIN_ABS};
constexpr Builtin FLOOR{"@floor", TokenType::BUILTIN_FLOOR};
constexpr Builtin CEIL{"@ceil", TokenType::BUILTIN_CEIL};

constexpr Builtin PANIC{"@panic", TokenType::BUILTIN_PANIC};

} // namespace builtins

constexpr auto ALL_BUILTINS = [] {
    auto all_builtins = std::array{
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
        builtins::PANIC,
    };

    std::ranges::sort(all_builtins, {}, &Builtin::first);
    return all_builtins;
}();

constexpr auto get_builtin(std::string_view sv) noexcept -> opt::Option<Builtin> {
    const auto it = std::ranges::lower_bound(ALL_BUILTINS, sv, {}, &Builtin::first);
    if (it == ALL_BUILTINS.end() || it->first != sv) { return opt::none; }
    return opt::Option<Builtin>{*it};
}

} // namespace porpoise::syntax
