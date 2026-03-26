#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class ArrayExpression : public ExprBase<ArrayExpression> {
  public:
    static constexpr auto KIND = NodeKind::ARRAY_EXPRESSION;

  public:
    explicit ArrayExpression(const syntax::Token&              start_token,
                             Optional<mem::Box<Expression>>    size,
                             ExplicitType&&                    item_type,
                             std::vector<mem::Box<Expression>> items) noexcept;
    ~ArrayExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ArrayExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(explicit_size, Expression, size_, **)
    MAKE_GETTER(item_type, const ExplicitType&)
    MAKE_GETTER(items, std::span<const mem::Box<Expression>>)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<mem::Box<Expression>>    size_;
    ExplicitType                      item_type_;
    std::vector<mem::Box<Expression>> items_;
};

} // namespace porpoise::ast
