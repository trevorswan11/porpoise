#pragma once

#include <cassert>
#include <charconv>
#include <string>
#include <utility>

#include <fmt/format.h>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

// cppcheck-suppress-begin [constParameterReference, duplInheritedMember]

// A necessarily instantiable Node with an underlying primitive value type.
template <typename N>
concept PrimitiveNode = LeafNode<N> && requires { typename N::value_type; };

// A primitive node without an explicit underlying payload
template <typename N>
concept LightPrimitiveNode = PrimitiveNode<N> && requires { typename N::is_light; };

template <typename ValueType, bool AssertLast = true>
[[nodiscard]] auto parse_primitive_value(std::string_view slice, syntax::TokenType type) noexcept
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
        assert(result.ptr == last);
        if (result.ec == std::errc{}) { return v; }
    } else {
        if (result.ec == std::errc{} && result.ptr == last) { return v; }
    }

    assert(result.ec == std::errc::result_out_of_range);
    return opt::none;
}

template <typename Derived, typename T> class PrimitiveExpression : public ExprBase<Derived> {
  public:
    using value_type = T;

  public:
    PrimitiveExpression(const syntax::Token& start_token, value_type value) noexcept
        : ExprBase<Derived>{start_token}, value_{std::move(value)} {}

    static auto parse(syntax::Parser& parser) -> Result<mem::Box<Expression>, syntax::Diagnostic>
        requires(!disable_default_parse<Derived>::value)
    {
        const auto start_token = parser.get_current_token();
        auto       value = parse_primitive_value<value_type>(start_token.slice, start_token.type);
        if (value) { return mem::make_box<Derived>(start_token, *value); }

        return make_syntax_err(std::is_same_v<value_type, f64>
                                   ? syntax::Error::DOUBLE_OVERFLOW
                                   : (std::is_same_v<value_type, f32>
                                          ? syntax::Error::FLOAT_OVERFLOW
                                          : syntax::Error::INTEGER_OVERFLOW),
                               start_token);
    }

    MAKE_GETTER(value, const value_type&)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = Node::as<Derived>(other);
        return value_ == casted.value_;
    }

  protected:
    value_type value_;
};

class StringExpression : public PrimitiveExpression<StringExpression, std::string> {
  public:
    static constexpr auto KIND = NodeKind::STRING_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(StringExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::Diagnostic>;
};
template <> struct disable_default_parse<StringExpression> : std::true_type {};

class I32Expression : public PrimitiveExpression<I32Expression, i32> {
  public:
    static constexpr auto KIND = NodeKind::I32_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(I32Expression)

    auto accept(Visitor& v) const -> void override;
};

class I64Expression : public PrimitiveExpression<I64Expression, i64> {
  public:
    static constexpr auto KIND = NodeKind::I64_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(I64Expression)

    auto accept(Visitor& v) const -> void override;
};

class ISizeExpression : public PrimitiveExpression<ISizeExpression, isize> {
  public:
    static constexpr auto KIND = NodeKind::ISIZE_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(ISizeExpression)

    auto accept(Visitor& v) const -> void override;
};

class U32Expression : public PrimitiveExpression<U32Expression, u32> {
  public:
    static constexpr auto KIND = NodeKind::U32_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(U32Expression)

    auto accept(Visitor& v) const -> void override;
};

class U64Expression : public PrimitiveExpression<U64Expression, u64> {
  public:
    static constexpr auto KIND = NodeKind::U64_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(U64Expression)

    auto accept(Visitor& v) const -> void override;
};

class USizeExpression : public PrimitiveExpression<USizeExpression, usize> {
  public:
    static constexpr auto KIND = NodeKind::USIZE_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(USizeExpression)

    auto accept(Visitor& v) const -> void override;
};

class U8Expression : public PrimitiveExpression<U8Expression, u8> {
  public:
    static constexpr auto KIND = NodeKind::U8_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(U8Expression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::Diagnostic>;
};
template <> struct disable_default_parse<U8Expression> : std::true_type {};

class F32Expression : public PrimitiveExpression<F32Expression, f32> {
  public:
    static constexpr auto KIND = NodeKind::F32_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(F32Expression)

    auto accept(Visitor& v) const -> void override;

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;
};

class F64Expression : public PrimitiveExpression<F64Expression, f64> {
  public:
    static constexpr auto KIND = NodeKind::F64_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(F64Expression)

    auto accept(Visitor& v) const -> void override;

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;
};

class BoolExpression : public ExprBase<BoolExpression> {
  public:
    using value_type = bool;
    using is_light   = void;

  public:
    static constexpr auto KIND = NodeKind::BOOL_EXPRESSION;

  public:
    explicit BoolExpression(const syntax::Token& start_token) noexcept : ExprBase{start_token} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(BoolExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::Diagnostic>;

    [[nodiscard]] auto get_value() const noexcept -> bool {
        return start_token_.type == syntax::TokenType::BOOLEAN_TRUE;
    }

  protected:
    auto is_equal(const Node&) const noexcept -> bool override { return true; }
};

class VoidExpression : public ExprBase<VoidExpression> {
  public:
    using value_type = Unit;
    using is_light   = void;

  public:
    static constexpr auto KIND = NodeKind::VOID_EXPRESSION;

  public:
    explicit VoidExpression(const syntax::Token& start_token) noexcept : ExprBase{start_token} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(VoidExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::Diagnostic>;

  protected:
    auto is_equal(const Node&) const noexcept -> bool override { return true; }
};

// cppcheck-suppress-end [constParameterReference, duplInheritedMember]

} // namespace porpoise::ast

template <porpoise::ast::PrimitiveNode P> struct fmt::formatter<P> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const P& p, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", p.get_value());
    }
};
