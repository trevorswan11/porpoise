#pragma once

#include <vector>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class ImplicitAccessExpression;

class Initializer {
  public:
    Initializer(mem::Box<ImplicitAccessExpression> member, mem::Box<Expression> value) noexcept;
    ~Initializer();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Initializer)

    auto accept(Visitor& v) const -> void;

    MAKE_GETTER(member, const ImplicitAccessExpression&, *)
    MAKE_GETTER(value, const Expression&, *)

    MAKE_EQ_DELEGATION(Initializer)

  private:
    mem::Box<ImplicitAccessExpression> member_;
    mem::Box<Expression>               value_;
};

class InitializerExpression : public ExprBase<InitializerExpression> {
  public:
    static constexpr auto KIND = NodeKind::INITIALIZER_EXPRESSION;

  public:
    InitializerExpression(const syntax::Token&         start_token,
                          mem::NullableBox<Expression> object_type,
                          std::vector<Initializer>     initializers) noexcept;
    ~InitializerExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(InitializerExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse_opt_object(syntax::Parser&              parser,
                                               mem::NullableBox<Expression> object = nullptr)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    // This is for the parser's dispatch table
    [[nodiscard]] static auto parse(syntax::Parser& parser, mem::Box<Expression> object)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_NULLABLE_BOX_UNPACKER(object_type, Expression, object_type_, *)
    [[nodiscard]] auto has_initializers() const noexcept -> bool { return !initializers_.empty(); }
    MAKE_GETTER(initializers, std::span<const Initializer>, )

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::NullableBox<Expression> object_type_;
    std::vector<Initializer>     initializers_;
};

} // namespace porpoise::ast
