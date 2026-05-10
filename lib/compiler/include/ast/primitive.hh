#pragma once

#include <type_traits>

#include "ast/handle.hh"

#include "syntax/error.hh"

namespace porpoise {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

#define DECLARE_PRIMITIVE_EXPRESSION(NodeType, ValueType)       \
    struct NodeType {                                           \
        using value_type = ValueType;                           \
        value_type                value;                        \
        [[nodiscard]] static auto parse(syntax::Parser& parser) \
            -> Result<ExpressionHandle, syntax::Diagnostic>;    \
    };

DECLARE_PRIMITIVE_EXPRESSION(StringExpression, std::string)
DECLARE_PRIMITIVE_EXPRESSION(I32Expression, i32)
DECLARE_PRIMITIVE_EXPRESSION(I64Expression, i64)
DECLARE_PRIMITIVE_EXPRESSION(ISizeExpression, isize)
DECLARE_PRIMITIVE_EXPRESSION(U32Expression, u32)
DECLARE_PRIMITIVE_EXPRESSION(U64Expression, u64)
DECLARE_PRIMITIVE_EXPRESSION(USizeExpression, usize)
DECLARE_PRIMITIVE_EXPRESSION(U8Expression, u8)
DECLARE_PRIMITIVE_EXPRESSION(F32Expression, f32)
DECLARE_PRIMITIVE_EXPRESSION(F64Expression, f64)

#undef DECLARE_PRIMITIVE_EXPRESSION

namespace traits {

template <typename T> struct is_valued_primitive : std::false_type {};
template <> struct is_valued_primitive<StringExpression> : std::true_type {};
template <> struct is_valued_primitive<I32Expression> : std::true_type {};
template <> struct is_valued_primitive<I64Expression> : std::true_type {};
template <> struct is_valued_primitive<ISizeExpression> : std::true_type {};
template <> struct is_valued_primitive<U32Expression> : std::true_type {};
template <> struct is_valued_primitive<U64Expression> : std::true_type {};
template <> struct is_valued_primitive<USizeExpression> : std::true_type {};
template <> struct is_valued_primitive<U8Expression> : std::true_type {};
template <> struct is_valued_primitive<F32Expression> : std::true_type {};
template <> struct is_valued_primitive<F64Expression> : std::true_type {};
template <typename T> constexpr auto is_primitive_v = is_valued_primitive<T>::value;

// A primitive node with its value embedded in the data
template <typename T>
concept ValuedPrimitiveNode = is_primitive_v<T>;

} // namespace traits

struct BoolExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct VoidExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

namespace traits {

template <typename T> struct is_light_primitive : std::false_type {};
template <> struct is_light_primitive<BoolExpression> : std::true_type {};
template <> struct is_light_primitive<VoidExpression> : std::true_type {};
template <typename T> constexpr auto is_light_primitive_v = is_light_primitive<T>::value;

// A primitive node with its value embedded in its id
template <typename T>
concept LightPrimitiveNode = is_light_primitive_v<T>;

} // namespace traits

} // namespace ast

} // namespace porpoise
