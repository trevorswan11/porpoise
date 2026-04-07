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
    InitializerExpression(const syntax::Token&           start_token,
                          Optional<mem::Box<Expression>> object,
                          std::vector<Initializer>       initializers) noexcept;
    ~InitializerExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(InitializerExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse_opt_object(syntax::Parser&                parser,
                                               Optional<mem::Box<Expression>> object = std::nullopt)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    // This is for the parser's dispatch table
    [[nodiscard]] static auto parse(syntax::Parser& parser, mem::Box<Expression> object)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(object, Expression, object_, **)
    MAKE_GETTER(initializers, std::span<const Initializer>, )

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<mem::Box<Expression>> object_;
    std::vector<Initializer>       initializers_;
};

} // namespace porpoise::ast
