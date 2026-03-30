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

template <typename T> struct disable_default_parse : std::false_type {};

// A necessarily instantiable Node with an underlying primitive value type.
template <typename N>
concept PrimitiveNode = LeafNode<N> && requires { typename N::value_type; };

template <typename Derived, typename T> class PrimitiveExpression : public ExprBase<Derived> {
  public:
    using value_type = T;

  public:
    PrimitiveExpression(const syntax::Token& start_token, value_type value) noexcept
        : ExprBase<Derived>{start_token}, value_{std::move(value)} {}

    static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>
        requires(!disable_default_parse<Derived>::value)
    {
        const auto start_token = parser.current_token();
        const auto base        = syntax::token_type::to_base(start_token.type);

        const auto* first =
            start_token.slice.cbegin() + (!base || *base == syntax::Base::DECIMAL ? 0 : 2);
        const auto* last =
            start_token.slice.cend() - syntax::token_type::suffix_length(start_token.type);

        value_type             v;
        std::from_chars_result result;
        if constexpr (std::is_same_v<value_type, f64> || std::is_same_v<value_type, f32>) {
            result = std::from_chars(first, last, v);
        } else {
            result = std::from_chars(first, last, v, std::to_underlying(*base));
        }
        if (result.ec == std::errc{} && result.ptr == last) {
            return mem::make_box<Derived>(start_token, v);
        }

        assert(result.ec == std::errc::result_out_of_range);
        return make_parser_unexpected(std::is_same_v<value_type, f64>
                                          ? syntax::ParserError::DOUBLE_OVERFLOW
                                          : (std::is_same_v<value_type, f32>
                                                 ? syntax::ParserError::FLOAT_OVERFLOW
                                                 : syntax::ParserError::INTEGER_OVERFLOW),
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
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;
};
template <> struct disable_default_parse<StringExpression> : std::true_type {};

class SignedIntegerExpression : public PrimitiveExpression<SignedIntegerExpression, i32> {
  public:
    static constexpr auto KIND = NodeKind::SIGNED_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(SignedIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class SignedLongIntegerExpression : public PrimitiveExpression<SignedLongIntegerExpression, i64> {
  public:
    static constexpr auto KIND = NodeKind::SIGNED_LONG_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(SignedLongIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class ISizeIntegerExpression : public PrimitiveExpression<ISizeIntegerExpression, isize> {
  public:
    static constexpr auto KIND = NodeKind::ISIZE_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(ISizeIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class UnsignedIntegerExpression : public PrimitiveExpression<UnsignedIntegerExpression, u32> {
  public:
    static constexpr auto KIND = NodeKind::UNSIGNED_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(UnsignedIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class UnsignedLongIntegerExpression
    : public PrimitiveExpression<UnsignedLongIntegerExpression, u64> {
  public:
    static constexpr auto KIND = NodeKind::UNSIGNED_LONG_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(UnsignedLongIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class USizeIntegerExpression : public PrimitiveExpression<USizeIntegerExpression, usize> {
  public:
    static constexpr auto KIND = NodeKind::USIZE_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(USizeIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class ByteExpression : public PrimitiveExpression<ByteExpression, byte> {
  public:
    static constexpr auto KIND = NodeKind::BYTE_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(ByteExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;
};
template <> struct disable_default_parse<ByteExpression> : std::true_type {};

class FloatExpression : public PrimitiveExpression<FloatExpression, f32> {
  public:
    static constexpr auto KIND = NodeKind::FLOAT_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(FloatExpression)

    auto accept(Visitor& v) const -> void override;

    auto is_equal(const Node& other) const noexcept -> bool override;
};

class DoubleExpression : public PrimitiveExpression<DoubleExpression, f64> {
  public:
    static constexpr auto KIND = NodeKind::DOUBLE_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(DoubleExpression)

    auto accept(Visitor& v) const -> void override;

    auto is_equal(const Node& other) const noexcept -> bool override;
};

class BoolExpression : public PrimitiveExpression<BoolExpression, bool> {
  public:
    static constexpr auto KIND = NodeKind::BOOL_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(BoolExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    operator bool() const noexcept { return value_; }
};
template <> struct disable_default_parse<BoolExpression> : std::true_type {};

// cppcheck-suppress-end [constParameterReference, duplInheritedMember]

} // namespace porpoise::ast

template <porpoise::ast::PrimitiveNode P> struct fmt::formatter<P> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const P& p, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", p.get_value());
    }
};
